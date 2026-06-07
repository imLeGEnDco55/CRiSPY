#include "drag_drop.h"
#include <shlwapi.h>

CDataObject::CDataObject(const wchar_t* pszFilePath) : m_cRef(1) {
    wcscpy_s(m_szFilePath, pszFilePath);
}

// IUnknown methods
STDMETHODIMP CDataObject::QueryInterface(REFIID riid, void** ppv) {
    if (riid == IID_IUnknown || riid == IID_IDataObject) {
        *ppv = static_cast<IDataObject*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDataObject::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDataObject::Release() {
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
        return 0;
    }
    return cRef;
}

// IDataObject methods
STDMETHODIMP CDataObject::GetData(FORMATETC* pfe, STGMEDIUM* pmed) {
    if (pfe->cfFormat == CF_HDROP && (pfe->tymed & TYMED_HGLOBAL)) {
        size_t len = wcslen(m_szFilePath);
        // Size requires DROPFILES struct plus two null-terminated string terminators (for the list end)
        size_t size = sizeof(DROPFILES) + (len + 2) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, size);
        if (!hGlobal) return E_OUTOFMEMORY;
        
        DROPFILES* pDrop = (DROPFILES*)GlobalLock(hGlobal);
        pDrop->pFiles = sizeof(DROPFILES);
        pDrop->fWide = TRUE;
        
        wchar_t* pFiles = (wchar_t*)((BYTE*)pDrop + sizeof(DROPFILES));
        wcscpy_s(pFiles, len + 1, m_szFilePath);
        pFiles[len + 1] = L'\0'; // Double null terminator to indicate end of list
        
        GlobalUnlock(hGlobal);
        
        pmed->tymed = TYMED_HGLOBAL;
        pmed->hGlobal = hGlobal;
        pmed->pUnkForRelease = NULL;
        return S_OK;
    }
    return DV_E_FORMATETC;
}

STDMETHODIMP CDataObject::GetDataHere(FORMATETC*, STGMEDIUM*) {
    return E_NOTIMPL;
}

STDMETHODIMP CDataObject::QueryGetData(FORMATETC* pfe) {
    if (pfe->cfFormat == CF_HDROP && (pfe->tymed & TYMED_HGLOBAL)) {
        return S_OK;
    }
    return DV_E_FORMATETC;
}

STDMETHODIMP CDataObject::GetCanonicalFormatEtc(FORMATETC*, FORMATETC* pfeOut) {
    pfeOut->ptd = NULL;
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CDataObject::SetData(FORMATETC*, STGMEDIUM*, BOOL) {
    return E_NOTIMPL;
}

STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDir, IEnumFORMATETC** ppenum) {
    if (dwDir == DATADIR_GET) {
        FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        return SHCreateStdEnumFmtEtc(1, &fmt, ppenum);
    }
    return E_NOTIMPL;
}

STDMETHODIMP CDataObject::DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) {
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDataObject::DUnadvise(DWORD) {
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDataObject::EnumDAdvise(IEnumSTATDATA**) {
    return OLE_E_ADVISENOTSUPPORTED;
}


CDropSource::CDropSource() : m_cRef(1) {}

// IUnknown methods
STDMETHODIMP CDropSource::QueryInterface(REFIID riid, void** ppv) {
    if (riid == IID_IUnknown || riid == IID_IDropSource) {
        *ppv = static_cast<IDropSource*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDropSource::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDropSource::Release() {
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
        return 0;
    }
    return cRef;
}

// IDropSource methods
STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) {
    if (fEscapePressed) return DRAGDROP_S_CANCEL;
    if (!(grfKeyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
    return S_OK;
}

STDMETHODIMP CDropSource::GiveFeedback(DWORD) {
    return DRAGDROP_S_USEDEFAULTCURSORS;
}
