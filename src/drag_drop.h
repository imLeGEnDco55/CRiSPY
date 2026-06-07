#pragma once
#include <windows.h>
#include <objidl.h>
#include <shlobj.h>

class CDataObject : public IDataObject {
private:
    ULONG m_cRef;
    wchar_t m_szFilePath[MAX_PATH];

public:
    CDataObject(const wchar_t* pszFilePath);
    virtual ~CDataObject() = default;

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDataObject methods
    STDMETHODIMP GetData(FORMATETC* pfe, STGMEDIUM* pmed) override;
    STDMETHODIMP GetDataHere(FORMATETC* pfe, STGMEDIUM* pmed) override;
    STDMETHODIMP QueryGetData(FORMATETC* pfe) override;
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pfeIn, FORMATETC* pfeOut) override;
    STDMETHODIMP SetData(FORMATETC* pfe, STGMEDIUM* pmed, BOOL fRelease) override;
    STDMETHODIMP EnumFormatEtc(DWORD dwDir, IEnumFORMATETC** ppenum) override;
    STDMETHODIMP DAdvise(FORMATETC* pfe, DWORD advf, IAdviseSink* pAdSink, DWORD* pdwConnection) override;
    STDMETHODIMP DUnadvise(DWORD dwConnection) override;
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppenumAdvise) override;
};

class CDropSource : public IDropSource {
private:
    ULONG m_cRef;

public:
    CDropSource();
    virtual ~CDropSource() = default;

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IDropSource methods
    STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override;
    STDMETHODIMP GiveFeedback(DWORD dwEffect) override;
};
