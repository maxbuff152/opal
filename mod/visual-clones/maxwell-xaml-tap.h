// =====================================================================
//  maxwell-xaml-tap.h
//
//  One XAML visual-tree tap for the whole Opal shell.
//
//  Written fresh against Microsoft's documented XAML diagnostics API, using
//  the interface declarations in the SDK's own <xamlom.h>. No upstream mod
//  code and no hand-copied interface declarations.
//
//  What this replaces:
//
//  The previous stack carried six private copies of a tap engine - one per
//  styler - and they had drifted into two incompatible families. Three mods
//  guarded the callback with a thread_local `g_initializedForThread`, three
//  with a global `GetCurrentThreadId() != g_targetThreadId`, and the guard
//  sat at a DIFFERENT POINT in the callback between the two, so some work ran
//  on the wrong thread in one family and not the other. A fix applied to one
//  copy never reached the other five. That is the single most likely source
//  of the Windows.UI.Xaml.dll faults in Explorer, and it is why this exists
//  exactly once.
//
//  How the tap gets in:
//
//  XAML exports InitializeXamlDiagnosticsEx from Windows.UI.Xaml.dll. Given a
//  TAP DLL path and a CLSID it will LoadLibrary that path and CoCreate the
//  class, then hand it an IXamlDiagnostics site. A Windhawk mod is not a
//  registered COM server and exports no DllGetClassObject, so we intercept
//  the load: while our own InitializeXamlDiagnosticsEx call is in flight we
//  answer LoadLibraryExW for our TAP path with our own module handle, and
//  serve the class ourselves. The interception is armed only for the duration
//  of that one call, on the calling thread, so we cannot affect unrelated
//  library loads in the host process.
// =====================================================================
#pragma once

#include <windows.h>
#include <ocidl.h>     // IObjectWithSite
#include <xamlom.h>

#include <atomic>
#include <string>

namespace MaxwellShell {

// Called once per element as it enters the visual tree.
//
// `diagnostics` is the live IXamlDiagnostics for this XAML thread, passed in so
// the handler can resolve the handle to a real object without the tap having to
// know anything about styling. It is borrowed - do not Release it.
//
// `type` and `name` are borrowed for the duration of the call; copy anything
// you keep. The call arrives on the XAML UI thread for this island.
using ElementCallback = void (*)(IXamlDiagnostics* diagnostics,
                                 InstanceHandle handle,
                                 const wchar_t* type,
                                 const wchar_t* name);

namespace detail {

// The tap object. Lives for as long as XAML holds a reference to it.
class Tap final : public IObjectWithSite, public IVisualTreeServiceCallback2 {
public:
    explicit Tap(ElementCallback cb) : m_callback(cb) {}

    // --- IUnknown -----------------------------------------------------
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) { return E_POINTER; }
        *ppv = nullptr;

        // __uuidof rather than the IID_* externs: those live in libuuid, and a
        // mod that maps into six processes should not carry a link dependency
        // it can resolve at compile time.
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IObjectWithSite)) {
            *ppv = static_cast<IObjectWithSite*>(this);
        } else if (riid == __uuidof(IVisualTreeServiceCallback) ||
                   riid == __uuidof(IVisualTreeServiceCallback2)) {
            *ppv = static_cast<IVisualTreeServiceCallback2*>(this);
        } else {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(m_refs.fetch_add(1, std::memory_order_relaxed) + 1);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = static_cast<ULONG>(m_refs.fetch_sub(1, std::memory_order_acq_rel) - 1);
        if (n == 0) { delete this; }
        return n;
    }

    // --- IObjectWithSite ----------------------------------------------
    // XAML hands us the diagnostics site here. This is also where we bind to
    // the tree, and - critically - where we record which thread owns this tap.
    HRESULT STDMETHODCALLTYPE SetSite(IUnknown* site) override {
        Unbind();
        if (!site) { return S_OK; }

        HRESULT hr = site->QueryInterface(__uuidof(IXamlDiagnostics),
                                          reinterpret_cast<void**>(&m_diagnostics));
        if (FAILED(hr)) { return hr; }

        hr = m_diagnostics->QueryInterface(__uuidof(IVisualTreeService3),
                                           reinterpret_cast<void**>(&m_service));
        if (FAILED(hr)) { Unbind(); return hr; }

        // One tap serves one XAML thread. Recording the owning thread HERE,
        // at bind time, is what lets OnVisualTreeChange reject foreign-thread
        // callbacks as its very first act - see the note there.
        m_ownerThread = GetCurrentThreadId();

        hr = m_service->AdviseVisualTreeChange(this);
        if (FAILED(hr)) { Unbind(); return hr; }

        m_bound = true;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppv) override {
        if (!ppv) { return E_POINTER; }
        *ppv = nullptr;
        if (!m_diagnostics) { return E_FAIL; }
        return m_diagnostics->QueryInterface(riid, ppv);
    }

    // --- IVisualTreeServiceCallback / ...2 -----------------------------
    HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation,
                                                 VisualElement element,
                                                 VisualMutationType mutationType) override {
        (void)relation;

        // FIRST statement, deliberately. XAML can call this on threads other
        // than the one we bound to, and every field of `element` belongs to
        // that other thread's tree. The old stack did this check roughly
        // twenty lines in, after already touching the element, which is a
        // race that only shows up as a crash under load.
        if (GetCurrentThreadId() != m_ownerThread) { return S_OK; }

        if (mutationType != Add) { return S_OK; }
        if (!m_callback || !m_diagnostics) { return S_OK; }

        // Re-entrancy guard. Applying a brush can cause XAML to realise a
        // template, which lands right back here on this same thread. Without
        // this the first templated control recurses until the stack dies.
        if (m_inCallback) { return S_OK; }
        m_inCallback = true;

        // BSTRs on the struct are owned by the caller; do not free them.
        // A styling handler must never be allowed to throw into XAML's
        // notification path, so it is fenced here rather than at each site.
        try {
            m_callback(m_diagnostics,
                       element.Handle,
                       element.Type ? element.Type : L"",
                       element.Name ? element.Name : L"");
        } catch (...) {
        }

        m_inCallback = false;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle,
                                                    VisualElementState,
                                                    LPCWSTR) override {
        return S_OK;
    }

    void Unbind() {
        if (m_service) {
            if (m_bound) { m_service->UnadviseVisualTreeChange(this); m_bound = false; }
            m_service->Release();
            m_service = nullptr;
        }
        if (m_diagnostics) { m_diagnostics->Release(); m_diagnostics = nullptr; }
    }

private:
    ~Tap() { Unbind(); }

    std::atomic<long>    m_refs{1};
    ElementCallback      m_callback   = nullptr;
    IXamlDiagnostics*    m_diagnostics = nullptr;
    IVisualTreeService3* m_service     = nullptr;
    DWORD                m_ownerThread = 0;
    bool                 m_bound       = false;
    bool                 m_inCallback  = false;  // guarded by m_ownerThread
};

// Minimal class factory. XAML CoCreates our CLSID exactly once per thread.
class TapFactory final : public IClassFactory {
public:
    explicit TapFactory(ElementCallback cb) : m_callback(cb) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) { return E_POINTER; }
        *ppv = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IClassFactory)) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(m_refs.fetch_add(1, std::memory_order_relaxed) + 1);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = static_cast<ULONG>(m_refs.fetch_sub(1, std::memory_order_acq_rel) - 1);
        if (n == 0) { delete this; }
        return n;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (!ppv) { return E_POINTER; }
        *ppv = nullptr;
        if (outer) { return CLASS_E_NOAGGREGATION; }

        auto* tap = new (std::nothrow) Tap(m_callback);
        if (!tap) { return E_OUTOFMEMORY; }

        const HRESULT hr = tap->QueryInterface(riid, ppv);
        tap->Release();   // factory drops its construction reference either way
        return hr;
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL) override { return S_OK; }

private:
    ~TapFactory() = default;
    std::atomic<long> m_refs{1};
    ElementCallback   m_callback = nullptr;
};

}  // namespace detail

}  // namespace MaxwellShell
