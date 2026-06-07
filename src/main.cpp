#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <cmath>
#include <wincodec.h>
#include "drag_drop.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Windowscodecs.lib")

// Custom Messages
#define WM_USER_FINISHED (WM_USER + 1)

// Timer IDs
#define TIMER_SPINNER 1

// UI Colors
#define CLR_BG_START      D2D1::ColorF(0x06060c) // Darker cyber black
#define CLR_BG_END        D2D1::ColorF(0x0f0f1c) // Dark space indigo
#define CLR_CARD_BG       D2D1::ColorF(0x121222) // Cyberpunk card bg
#define CLR_CARD_BORDER   D2D1::ColorF(0x22223a) // Dark slate border

// User-Requested Neon Palette
#define CLR_LIME_GREEN    D2D1::ColorF(0x32CD32) // Lime Green (#32CD32)
#define CLR_HOT_PINK      D2D1::ColorF(0xFF69B4) // Hot Pink (#FF69B4)
#define CLR_CORAL         D2D1::ColorF(0xFF7F50) // Coral (#FF7F50)
#define CLR_CYAN          D2D1::ColorF(0x00FFFF) // Cyan (#00FFFF)
#define CLR_ROYAL_BLUE    D2D1::ColorF(0x4169E1) // Royal Blue (#4169E1)

#define CLR_TEXT_WHITE    D2D1::ColorF(0xFFFFFF)
#define CLR_TEXT_GRAY     D2D1::ColorF(0x8e8e9f)
#define CLR_TEXT_ERROR    CLR_CORAL

// App States
enum AppState {
    STATE_KEY_INPUT,
    STATE_READY,
    STATE_PROCESSING,
    STATE_COMPLETE,
    STATE_ERROR
};

// Global variables
HWND g_hwndEditKey = NULL;
HWND g_hwndMain = NULL;
HWND g_hwndFLStudio = NULL;
bool g_isAttachedToFL = false;
AppState g_appState = STATE_KEY_INPUT;
std::wstring g_apiKey = L"";
std::wstring g_inputFilePath = L"";
std::wstring g_cleanFilePath = L"";
std::wstring g_errorMessage = L"";
bool g_isEditFocused = false;
float g_spinnerAngle = 0.0f;
float g_gridOffset = 0.0f;
bool g_isDraggingOut = false;

// Direct2D Interfaces
ID2D1Factory* g_pD2DFactory = NULL;
ID2D1HwndRenderTarget* g_pRenderTarget = NULL;
ID2D1LinearGradientBrush* g_pBgBrush = NULL;
ID2D1SolidColorBrush* g_pBrushCardBg = NULL;
ID2D1SolidColorBrush* g_pBrushCardBorder = NULL;
ID2D1SolidColorBrush* g_pBrushTextWhite = NULL;
ID2D1SolidColorBrush* g_pBrushTextGray = NULL;
ID2D1SolidColorBrush* g_pBrushTextError = NULL;
ID2D1StrokeStyle* g_pDashedStroke = NULL;

// New Neon Brushes
ID2D1SolidColorBrush* g_pBrushLimeGreen = NULL;
ID2D1SolidColorBrush* g_pBrushHotPink = NULL;
ID2D1SolidColorBrush* g_pBrushCoral = NULL;
ID2D1SolidColorBrush* g_pBrushCyan = NULL;
ID2D1SolidColorBrush* g_pBrushRoyalBlue = NULL;
ID2D1SolidColorBrush* g_pBrushGrid = NULL;

// WIC & Image Loading
IWICImagingFactory* g_pWICFactory = NULL;
ID2D1Bitmap* g_pLogoBitmap = NULL;

// DirectWrite Interfaces
IDWriteFactory* g_pDWriteFactory = NULL;
IDWriteTextFormat* g_pFormatTitle = NULL;
IDWriteTextFormat* g_pFormatHeader = NULL;
IDWriteTextFormat* g_pFormatNormal = NULL;
IDWriteTextFormat* g_pFormatSmall = NULL;

// UI Layout Rects (calculated dynamically or statically)
D2D1_RECT_F g_rectBtnSave = {};
D2D1_RECT_F g_rectBtnReset = {};
D2D1_RECT_F g_rectBtnAgain = {};
D2D1_RECT_F g_rectDragCard = {};

// Helper: Get config file path in AppData
std::wstring GetConfigFilePath() {
    wchar_t szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        std::wstring wsPath(szPath);
        wsPath += L"\\NvidiaStudioVoiceEnhancer";
        CreateDirectoryW(wsPath.c_str(), NULL);
        wsPath += L"\\config.txt";
        return wsPath;
    }
    return L"config.txt";
}

// Save API Key to file
void SaveApiKey(const std::wstring& key) {
    std::wstring path = GetConfigFilePath();
    std::wofstream file(path);
    if (file.is_open()) {
        file << key;
        file.close();
    }
}

// Load API Key from file
std::wstring LoadApiKey() {
    std::wstring path = GetConfigFilePath();
    std::wifstream file(path);
    if (file.is_open()) {
        std::wstring key;
        std::getline(file, key);
        file.close();
        return key;
    }
    return L"";
}

// Helper to dynamically locate the logo PNG image
std::wstring LocateLogoImage() {
    wchar_t szExePath[MAX_PATH];
    GetModuleFileNameW(NULL, szExePath, MAX_PATH);
    PathRemoveFileSpecW(szExePath); // Folder of executable
    
    // Check 1: Next to executable
    std::wstring path1 = std::wstring(szExePath) + L"\\Crispy.png";
    if (PathFileExistsW(path1.c_str())) return path1;
    
    // Check 2: One level up
    std::wstring path2 = std::wstring(szExePath) + L"\\..\\Crispy.png";
    if (PathFileExistsW(path2.c_str())) return path2;
    
    // Check 3: Two levels up
    std::wstring path3 = std::wstring(szExePath) + L"\\..\\..\\Crispy.png";
    if (PathFileExistsW(path3.c_str())) return path3;
    
    // Check 4: Workspace root directly
    std::wstring path4 = L"c:\\Apps\\CRiSPY\\Crispy.png";
    if (PathFileExistsW(path4.c_str())) return path4;
    
    // Fallback
    return L"Crispy.png";
}

// Helper to load bitmap from PNG via WIC
HRESULT LoadBitmapFromFile(
    ID2D1RenderTarget* pRenderTarget,
    IWICImagingFactory* pIWICFactory,
    PCWSTR uri,
    UINT destinationWidth,
    UINT destinationHeight,
    ID2D1Bitmap** ppBitmap
) {
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICFormatConverter* pConverter = NULL;

    HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
        uri,
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &pDecoder
    );

    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }

    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(
            pSource,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeMedianCut
        );
    }

    if (SUCCEEDED(hr)) {
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            pConverter,
            NULL,
            ppBitmap
        );
    }

    if (pDecoder) pDecoder->Release();
    if (pSource) pSource->Release();
    if (pConverter) pConverter->Release();

    return hr;
}

// Initialize Direct2D and DirectWrite resources
HRESULT InitD2D(HWND hWnd) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
    if (FAILED(hr)) return hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&g_pDWriteFactory));
    if (FAILED(hr)) return hr;

    // Create text formats
    g_pDWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 32.0f, L"en-us", &g_pFormatTitle);
    g_pDWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"en-us", &g_pFormatHeader);
    g_pDWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"en-us", &g_pFormatNormal);
    g_pDWriteFactory->CreateTextFormat(L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 11.0f, L"en-us", &g_pFormatSmall);

    if (g_pFormatTitle) g_pFormatTitle->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    if (g_pFormatHeader) g_pFormatHeader->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    if (g_pFormatNormal) g_pFormatNormal->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    if (g_pFormatSmall) g_pFormatSmall->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    // Dashed border style
    D2D1_STROKE_STYLE_PROPERTIES strokeProps = {};
    strokeProps.dashStyle = D2D1_DASH_STYLE_DASH;
    g_pD2DFactory->CreateStrokeStyle(&strokeProps, NULL, 0, &g_pDashedStroke);

    // Initialize WIC Factory for PNG support
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&g_pWICFactory)
    );

    return S_OK;
}

// Discard device resources
void DiscardDeviceResources() {
    if (g_pBgBrush) { g_pBgBrush->Release(); g_pBgBrush = NULL; }
    if (g_pBrushCardBg) { g_pBrushCardBg->Release(); g_pBrushCardBg = NULL; }
    if (g_pBrushCardBorder) { g_pBrushCardBorder->Release(); g_pBrushCardBorder = NULL; }
    if (g_pBrushTextWhite) { g_pBrushTextWhite->Release(); g_pBrushTextWhite = NULL; }
    if (g_pBrushTextGray) { g_pBrushTextGray->Release(); g_pBrushTextGray = NULL; }
    if (g_pBrushTextError) { g_pBrushTextError->Release(); g_pBrushTextError = NULL; }
    
    if (g_pBrushLimeGreen) { g_pBrushLimeGreen->Release(); g_pBrushLimeGreen = NULL; }
    if (g_pBrushHotPink) { g_pBrushHotPink->Release(); g_pBrushHotPink = NULL; }
    if (g_pBrushCoral) { g_pBrushCoral->Release(); g_pBrushCoral = NULL; }
    if (g_pBrushCyan) { g_pBrushCyan->Release(); g_pBrushCyan = NULL; }
    if (g_pBrushRoyalBlue) { g_pBrushRoyalBlue->Release(); g_pBrushRoyalBlue = NULL; }
    if (g_pBrushGrid) { g_pBrushGrid->Release(); g_pBrushGrid = NULL; }

    if (g_pLogoBitmap) { g_pLogoBitmap->Release(); g_pLogoBitmap = NULL; }

    if (g_pRenderTarget) { g_pRenderTarget->Release(); g_pRenderTarget = NULL; }
}

// Create device-dependent resources
HRESULT CreateDeviceResources(HWND hWnd) {
    if (g_pRenderTarget) return S_OK;

    RECT rc;
    GetClientRect(hWnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    HRESULT hr = g_pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hWnd, size),
        &g_pRenderTarget
    );
    if (FAILED(hr)) return hr;

    // Create solid brushes
    g_pRenderTarget->CreateSolidColorBrush(CLR_CARD_BG, &g_pBrushCardBg);
    g_pRenderTarget->CreateSolidColorBrush(CLR_CARD_BORDER, &g_pBrushCardBorder);
    g_pRenderTarget->CreateSolidColorBrush(CLR_TEXT_WHITE, &g_pBrushTextWhite);
    g_pRenderTarget->CreateSolidColorBrush(CLR_TEXT_GRAY, &g_pBrushTextGray);
    g_pRenderTarget->CreateSolidColorBrush(CLR_TEXT_ERROR, &g_pBrushTextError);

    // Create user color palette brushes
    g_pRenderTarget->CreateSolidColorBrush(CLR_LIME_GREEN, &g_pBrushLimeGreen);
    g_pRenderTarget->CreateSolidColorBrush(CLR_HOT_PINK, &g_pBrushHotPink);
    g_pRenderTarget->CreateSolidColorBrush(CLR_CORAL, &g_pBrushCoral);
    g_pRenderTarget->CreateSolidColorBrush(CLR_CYAN, &g_pBrushCyan);
    g_pRenderTarget->CreateSolidColorBrush(CLR_ROYAL_BLUE, &g_pBrushRoyalBlue);
    g_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x4169E1, 0.08f), &g_pBrushGrid); // Royal Blue with low opacity for cyber grid

    // Create background gradient brush
    D2D1_GRADIENT_STOP stops[] = {
        { 0.0f, CLR_BG_START },
        { 1.0f, CLR_BG_END }
    };
    ID2D1GradientStopCollection* pStops = NULL;
    hr = g_pRenderTarget->CreateGradientStopCollection(stops, 2, &pStops);
    if (SUCCEEDED(hr)) {
        g_pRenderTarget->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0.0f, 0.0f),
                D2D1::Point2F(0.0f, (float)size.height)
            ),
            pStops,
            &g_pBgBrush
        );
        pStops->Release();
    }

    // Load PNG logo image using WIC factory
    if (g_pWICFactory && !g_pLogoBitmap) {
        std::wstring logoPath = LocateLogoImage();
        LoadBitmapFromFile(g_pRenderTarget, g_pWICFactory, logoPath.c_str(), 0, 0, &g_pLogoBitmap);
    }

    return S_OK;
}

// Helper to draw PNG title logo at top
void DrawLogo(float centerX, float centerY) {
    if (g_pLogoBitmap) {
        D2D1_SIZE_F bitmapSize = g_pLogoBitmap->GetSize();
        // Constrain image width to 300 pixels and scale height proportionally
        float destW = 300.0f;
        float destH = bitmapSize.height * (destW / bitmapSize.width);
        float destX = centerX - destW / 2.0f;
        float destY = centerY - destH / 2.0f;
        g_pRenderTarget->DrawBitmap(
            g_pLogoBitmap,
            D2D1::RectF(destX, destY, destX + destW, destY + destH)
        );
    } else {
        // Fallback to stylized text if logo is not found
        const wchar_t* titleText = L"CRiSPY";
        g_pRenderTarget->DrawText(
            titleText,
            (UINT32)wcslen(titleText),
            g_pFormatTitle,
            D2D1::RectF(centerX - 100.0f, centerY - 25.0f, centerX + 100.0f, centerY + 25.0f),
            g_pBrushLimeGreen
        );
    }
}

// Draw checkmark check symbol inside a circle (OK Icon)
void DrawCheckmarkIcon(float cx, float cy, float radius, bool filled) {
    if (filled) {
        g_pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), g_pBrushLimeGreen);
        // Add futuristic neon glow outer ring
        g_pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius + 4.0f, radius + 4.0f), g_pBrushLimeGreen, 1.0f);
    } else {
        g_pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), g_pBrushLimeGreen, 2.5f);
    }

    // Draw check lines
    g_pRenderTarget->DrawLine(
        D2D1::Point2F(cx - radius * 0.4f, cy + radius * 0.05f),
        D2D1::Point2F(cx - radius * 0.05f, cy + radius * 0.35f),
        g_pBrushTextWhite,
        3.0f
    );
    g_pRenderTarget->DrawLine(
        D2D1::Point2F(cx - radius * 0.05f, cy + radius * 0.35f),
        D2D1::Point2F(cx + radius * 0.45f, cy - radius * 0.25f),
        g_pBrushTextWhite,
        3.0f
    );
}

// Draw a minimalist Restart/Refresh circular arrow icon
void DrawRestartIcon(float cx, float cy, float radius, bool hover) {
    ID2D1SolidColorBrush* brush = hover ? g_pBrushHotPink : g_pBrushCyan;
    
    // Rotate the arrow head dynamically on hover for a premium micro-animation
    D2D1_MATRIX_3X2_F originalTransform;
    g_pRenderTarget->GetTransform(&originalTransform);
    if (hover) {
        float rot = (float)GetTickCount() * 0.15f;
        g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(rot, D2D1::Point2F(cx, cy)) * originalTransform);
    }

    // Draw circular path with a gap
    g_pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), brush, 2.0f);

    // Draw arrow head
    g_pRenderTarget->DrawLine(D2D1::Point2F(cx + radius - 4, cy - 2), D2D1::Point2F(cx + radius + 1, cy + 3), brush, 2.0f);
    g_pRenderTarget->DrawLine(D2D1::Point2F(cx + radius + 5, cy - 2), D2D1::Point2F(cx + radius + 1, cy + 3), brush, 2.0f);

    if (hover) {
        g_pRenderTarget->SetTransform(originalTransform);
    }
}

// Draw modern Soundwave equalizer icon (animated dynamically!)
void DrawSoundwaveIcon(float centerX, float centerY) {
    float barWidth = 8.0f;
    float gap = 6.0f;
    
    // Animate bar heights using sine waves to make the interface feel alive
    float time = (float)GetTickCount() / 1000.0f;
    float baseHeights[] = { 24.0f, 60.0f, 100.0f, 60.0f, 24.0f };
    float heights[5];
    for (int i = 0; i < 5; ++i) {
        float speed = (g_appState == STATE_PROCESSING) ? 14.0f : 3.5f;
        float amp = (g_appState == STATE_PROCESSING) ? 30.0f : 8.0f;
        heights[i] = baseHeights[i] + sinf(time * speed + i * 1.5f) * amp;
        if (heights[i] < 8.0f) heights[i] = 8.0f;
    }
    
    for (int i = 0; i < 5; ++i) {
        float x = centerX + (i - 2) * (barWidth + gap);
        float h = heights[i];
        D2D1_ROUNDED_RECT rRect = D2D1::RoundedRect(
            D2D1::RectF(x - barWidth/2, centerY - h/2, x + barWidth/2, centerY + h/2),
            4.0f, 4.0f
        );
        // Multi-color palette assignment for cyberpunk futuristic visual
        ID2D1SolidColorBrush* brush = g_pBrushLimeGreen;
        if (i == 0 || i == 4) brush = g_pBrushRoyalBlue;
        else if (i == 1 || i == 3) brush = g_pBrushCyan;
        g_pRenderTarget->FillRoundedRectangle(rRect, brush);
    }
}

// Draw Paper Document File icon with neon border
void DrawFileIcon(float centerX, float centerY, bool glow) {
    float width = 70.0f;
    float height = 98.0f;
    float fold = 20.0f;

    D2D1_RECT_F outer = D2D1::RectF(centerX - width/2, centerY - height/2, centerX + width/2, centerY + height/2);

    // Main file background
    g_pRenderTarget->FillRectangle(outer, g_pBrushCardBg);
    g_pRenderTarget->DrawRectangle(outer, glow ? g_pBrushHotPink : g_pBrushCyan, 2.0f);

    // Fold
    D2D1_POINT_2F foldPoints[] = {
        D2D1::Point2F(centerX + width/2 - fold, centerY - height/2),
        D2D1::Point2F(centerX + width/2 - fold, centerY - height/2 + fold),
        D2D1::Point2F(centerX + width/2, centerY - height/2 + fold)
    };
    g_pRenderTarget->DrawLine(foldPoints[0], foldPoints[1], glow ? g_pBrushHotPink : g_pBrushCyan, 2.0f);
    g_pRenderTarget->DrawLine(foldPoints[1], foldPoints[2], glow ? g_pBrushHotPink : g_pBrushCyan, 2.0f);

    // Lines inside
    g_pRenderTarget->DrawLine(D2D1::Point2F(centerX - 18, centerY - 12), D2D1::Point2F(centerX + 18, centerY - 12), g_pBrushRoyalBlue, 2.5f);
    g_pRenderTarget->DrawLine(D2D1::Point2F(centerX - 18, centerY), D2D1::Point2F(centerX + 8, centerY), g_pBrushRoyalBlue, 2.5f);
    g_pRenderTarget->DrawLine(D2D1::Point2F(centerX - 18, centerY + 12), D2D1::Point2F(centerX + 18, centerY + 12), g_pBrushRoyalBlue, 2.5f);

    // Checkmark inside file
    if (g_appState == STATE_COMPLETE) {
        float cx = centerX + 18.0f;
        float cy = centerY + 30.0f;
        g_pRenderTarget->DrawLine(D2D1::Point2F(cx - 7, cy), D2D1::Point2F(cx - 1, cy + 4), g_pBrushLimeGreen, 3.0f);
        g_pRenderTarget->DrawLine(D2D1::Point2F(cx - 1, cy + 4), D2D1::Point2F(cx + 7, cy - 4), g_pBrushLimeGreen, 3.0f);
    }
}

// Render the application interface
void Render(HWND hWnd) {
    if (FAILED(CreateDeviceResources(hWnd))) return;

    g_pRenderTarget->BeginDraw();
    
    // Gradient Background
    D2D1_SIZE_F rtSize = g_pRenderTarget->GetSize();
    if (g_pBgBrush) {
        g_pRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, rtSize.width, rtSize.height), g_pBgBrush);
    }

    // Draw futuristic moving background grid lines
    if (g_pBrushGrid) {
        float gridSpacing = 40.0f;
        float startY = fmodf(g_gridOffset, gridSpacing);
        for (float y = startY; y < rtSize.height; y += gridSpacing) {
            g_pRenderTarget->DrawLine(
                D2D1::Point2F(0.0f, y),
                D2D1::Point2F(rtSize.width, y),
                g_pBrushGrid,
                0.5f
            );
        }
        for (float x = 0.0f; x < rtSize.width; x += gridSpacing) {
            g_pRenderTarget->DrawLine(
                D2D1::Point2F(x, 0.0f),
                D2D1::Point2F(x, rtSize.height),
                g_pBrushGrid,
                0.5f
            );
        }
    }

    POINT ptCursor;
    GetCursorPos(&ptCursor);
    ScreenToClient(hWnd, &ptCursor);
    float mouseX = (float)ptCursor.x;
    float mouseY = (float)ptCursor.y;

    if (g_appState == STATE_KEY_INPUT) {
        // --- SCREEN 1: Simple API input ---
        // Edit control custom background card
        float editX = 50.0f;
        float editY = 150.0f;
        float editW = 300.0f;
        float editH = 36.0f;
        
        D2D1_ROUNDED_RECT editBorder = D2D1::RoundedRect(
            D2D1::RectF(editX, editY, editX + editW, editY + editH),
            6.0f, 6.0f
        );
        g_pRenderTarget->FillRoundedRectangle(editBorder, g_pBrushCardBg);
        
        ID2D1SolidColorBrush* borderBrush = g_isEditFocused ? g_pBrushCyan : g_pBrushRoyalBlue;
        g_pRenderTarget->DrawRoundedRectangle(editBorder, borderBrush, g_isEditFocused ? 2.0f : 1.0f);

        // Positioning edit box
        if (g_hwndEditKey) {
            MoveWindow(g_hwndEditKey, (int)(editX + 8), (int)(editY + 8), (int)(editW - 16), (int)(editH - 16), TRUE);
            ShowWindow(g_hwndEditKey, SW_SHOW);
        }

        // Save Button (OK Icon) below the edit box
        float btnCx = 200.0f;
        float btnCy = 230.0f;
        float btnRadius = 24.0f;
        g_rectBtnSave = D2D1::RectF(btnCx - btnRadius, btnCy - btnRadius, btnCx + btnRadius, btnCy + btnRadius);

        bool hoverSave = (mouseX >= g_rectBtnSave.left && mouseX <= g_rectBtnSave.right && mouseY >= g_rectBtnSave.top && mouseY <= g_rectBtnSave.bottom);
        
        // Add subtle pulsing effect to the radius if not hovered to attract user interaction
        float pulse = hoverSave ? 0.0f : sinf((float)GetTickCount() * 0.006f) * 1.5f;
        
        // Draw green circle checkmark
        DrawCheckmarkIcon(btnCx, btnCy, btnRadius + pulse, hoverSave);

    } else {
        // Hide edit key box
        if (g_hwndEditKey) {
            ShowWindow(g_hwndEditKey, SW_HIDE);
        }

        if (g_appState == STATE_READY) {
            // --- SCREEN 2: CRiSPY PNG Logo & Drop Zone ---
            DrawLogo(200.0f, 60.0f);

            // Circular Drop Zone
            float circleX = 200.0f;
            float circleY = 235.0f;
            float circleRadius = 80.0f;
            
            bool hoverCard = (std::sqrt((mouseX - circleX)*(mouseX - circleX) + (mouseY - circleY)*(mouseY - circleY)) <= circleRadius);

            // Futuristic double outer rotating dashboard rings
            D2D1_MATRIX_3X2_F originalTransform;
            g_pRenderTarget->GetTransform(&originalTransform);
            
            // Outer Ring 1 (rotates clockwise)
            g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(g_spinnerAngle, D2D1::Point2F(circleX, circleY)) * originalTransform);
            g_pRenderTarget->DrawEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX, circleY), circleRadius + 6.0f, circleRadius + 6.0f), 
                g_pBrushRoyalBlue, 
                1.5f, 
                g_pDashedStroke
            );
            
            // Outer Ring 2 (rotates counter-clockwise)
            g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(-g_spinnerAngle * 1.5f, D2D1::Point2F(circleX, circleY)) * originalTransform);
            g_pRenderTarget->DrawEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX, circleY), circleRadius + 12.0f, circleRadius + 12.0f), 
                g_pBrushCyan, 
                1.0f, 
                g_pDashedStroke
            );

            // Restore normal transform
            g_pRenderTarget->SetTransform(originalTransform);

            // Main center circle (glowing cyan when hovered)
            float drawRadius = circleRadius;
            if (hoverCard) {
                drawRadius += sinf((float)GetTickCount() * 0.01f) * 2.0f;
            }

            g_pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(circleX, circleY), drawRadius, drawRadius), g_pBrushCardBg);
            g_pRenderTarget->DrawEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX, circleY), drawRadius, drawRadius), 
                hoverCard ? g_pBrushCyan : g_pBrushCardBorder, 
                hoverCard ? 2.5f : 1.5f
            );

            // Soundwave Icon inside
            DrawSoundwaveIcon(circleX, circleY);

        } else if (g_appState == STATE_PROCESSING) {
            // --- SCREEN 3: CRiSPY PNG Logo & Radar Scanning Loader ---
            DrawLogo(200.0f, 60.0f);

            // Orbiting Loader
            float circleX = 200.0f;
            float circleY = 235.0f;
            float circleRadius = 80.0f;
            
            g_pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(circleX, circleY), circleRadius, circleRadius), g_pBrushCardBg);
            g_pRenderTarget->DrawEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX, circleY), circleRadius, circleRadius), 
                g_pBrushCardBorder, 
                1.5f
            );

            // Outer dual-spinner orbits
            D2D1_MATRIX_3X2_F originalTransform;
            g_pRenderTarget->GetTransform(&originalTransform);
            
            // Orbit 1: Cyan (clockwise)
            g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(g_spinnerAngle, D2D1::Point2F(circleX, circleY)) * originalTransform);
            g_pRenderTarget->DrawEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX, circleY), circleRadius + 6.0f, circleRadius + 6.0f), 
                g_pBrushCyan, 
                1.0f
            );
            g_pRenderTarget->FillEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX + circleRadius + 6.0f, circleY), 5.0f, 5.0f),
                g_pBrushCyan
            );

            // Orbit 2: Hot Pink (counter-clockwise)
            g_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(-g_spinnerAngle * 1.5f, D2D1::Point2F(circleX, circleY)) * originalTransform);
            g_pRenderTarget->DrawEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX, circleY), circleRadius + 12.0f, circleRadius + 12.0f), 
                g_pBrushHotPink, 
                1.0f
            );
            g_pRenderTarget->FillEllipse(
                D2D1::Ellipse(D2D1::Point2F(circleX + circleRadius + 12.0f, circleY), 4.0f, 4.0f),
                g_pBrushHotPink
            );

            g_pRenderTarget->SetTransform(originalTransform);

            // Sweeping Scanner Line (Cyan line sliding inside circle)
            float sweepSpeed = 0.0035f;
            float sweepProgress = sinf((float)GetTickCount() * sweepSpeed);
            float sweepY = circleY + sweepProgress * (circleRadius - 5.0f);
            
            float lineHalfWidth = sqrtf(circleRadius * circleRadius - (sweepY - circleY) * (sweepY - circleY));
            g_pRenderTarget->DrawLine(
                D2D1::Point2F(circleX - lineHalfWidth, sweepY),
                D2D1::Point2F(circleX + lineHalfWidth, sweepY),
                g_pBrushCyan,
                2.0f
            );

            // Soundwave inside (rapidly animating)
            DrawSoundwaveIcon(circleX, circleY);

        } else if (g_appState == STATE_COMPLETE) {
            // --- SCREEN 4: Complete State (OK Top, Glowing File Middle, Reset Bottom-Right) ---
            // OK Icon on Top (Glowing Lime Green Checkmark)
            DrawCheckmarkIcon(200.0f, 60.0f, 24.0f, true);

            // Center Card (Draggable File)
            float cardW = 160.0f;
            float cardH = 160.0f;
            g_rectDragCard = D2D1::RectF(200.0f - cardW/2, 235.0f - cardH/2, 200.0f + cardW/2, 235.0f + cardH/2);
            
            bool hoverDrag = (mouseX >= g_rectDragCard.left && mouseX <= g_rectDragCard.right && mouseY >= g_rectDragCard.top && mouseY <= g_rectDragCard.bottom);
            
            D2D1_ROUNDED_RECT innerRect = D2D1::RoundedRect(g_rectDragCard, 8.0f, 8.0f);
            g_pRenderTarget->FillRoundedRectangle(innerRect, g_pBrushCardBg);
            
            // Border changes from Cyan to Hot Pink on hover
            ID2D1SolidColorBrush* borderBrush = hoverDrag ? g_pBrushHotPink : g_pBrushCyan;
            g_pRenderTarget->DrawRoundedRectangle(innerRect, borderBrush, hoverDrag ? 2.5f : 1.5f);

            // Futuristic corner accents on drag card
            float accentOffset = 12.0f;
            // Top-left
            g_pRenderTarget->DrawLine(D2D1::Point2F(g_rectDragCard.left, g_rectDragCard.top + accentOffset), D2D1::Point2F(g_rectDragCard.left, g_rectDragCard.top), borderBrush, 2.0f);
            g_pRenderTarget->DrawLine(D2D1::Point2F(g_rectDragCard.left, g_rectDragCard.top), D2D1::Point2F(g_rectDragCard.left + accentOffset, g_rectDragCard.top), borderBrush, 2.0f);
            // Bottom-right
            g_pRenderTarget->DrawLine(D2D1::Point2F(g_rectDragCard.right, g_rectDragCard.bottom - accentOffset), D2D1::Point2F(g_rectDragCard.right, g_rectDragCard.bottom), borderBrush, 2.0f);
            g_pRenderTarget->DrawLine(D2D1::Point2F(g_rectDragCard.right, g_rectDragCard.bottom), D2D1::Point2F(g_rectDragCard.right - accentOffset, g_rectDragCard.bottom), borderBrush, 2.0f);

            DrawFileIcon(200.0f, 235.0f, hoverDrag);

            // Restart Icon at bottom right
            float restX = 350.0f;
            float restY = 350.0f;
            float restR = 16.0f;
            g_rectBtnReset = D2D1::RectF(restX - restR, restY - restR, restX + restR, restY + restR);

            bool hoverReset = (mouseX >= g_rectBtnReset.left && mouseX <= g_rectBtnReset.right && mouseY >= g_rectBtnReset.top && mouseY <= g_rectBtnReset.bottom);

            DrawRestartIcon(restX, restY, restR, hoverReset);

        } else if (g_appState == STATE_ERROR) {
            // --- ERROR SCREEN ---
            const wchar_t* errorHeader = L"ERROR";
            g_pRenderTarget->DrawText(
                errorHeader,
                (UINT32)wcslen(errorHeader),
                g_pFormatTitle,
                D2D1::RectF(0.0f, 40.0f, rtSize.width, 100.0f),
                g_pBrushCoral
            );

            // Card error message
            float cardW = 320.0f;
            float cardH = 140.0f;
            float cardX = (rtSize.width - cardW) / 2;
            float cardY = 120.0f;
            
            D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(D2D1::RectF(cardX, cardY, cardX + cardW, cardY + cardH), 8.0f, 8.0f);
            g_pRenderTarget->FillRoundedRectangle(cardRect, g_pBrushCardBg);
            g_pRenderTarget->DrawRoundedRectangle(cardRect, g_pBrushCoral, 1.5f); // Coral error border

            g_pRenderTarget->DrawText(
                g_errorMessage.c_str(),
                (UINT32)g_errorMessage.length(),
                g_pFormatSmall,
                D2D1::RectF(cardX + 10.0f, cardY + 15.0f, cardX + cardW - 10.0f, cardY + cardH - 10.0f),
                g_pBrushTextWhite
            );

            // Retry Button at bottom
            float btnCx = 200.0f;
            float btnCy = 295.0f;
            float btnR = 18.0f;
            g_rectBtnAgain = D2D1::RectF(btnCx - btnR, btnCy - btnR, btnCx + btnR, btnCy + btnR);

            bool hoverAgain = (mouseX >= g_rectBtnAgain.left && mouseX <= g_rectBtnAgain.right && mouseY >= g_rectBtnAgain.top && mouseY <= g_rectBtnAgain.bottom);

            DrawRestartIcon(btnCx, btnCy, btnR, hoverAgain);
        }
    }

    // Draw bottom signature centered in white
    const wchar_t* signatureText = L"2026 - imLeGEnDco. - Vicodeado en Antigravity con Gemini";
    g_pRenderTarget->DrawText(
        signatureText,
        (UINT32)wcslen(signatureText),
        g_pFormatSmall,
        D2D1::RectF(0.0f, 378.0f, rtSize.width, 393.0f),
        g_pBrushTextWhite
    );

    HRESULT hr = g_pRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }
}

// Helper to dynamically locate the python.exe executable
std::wstring LocatePythonExecutable() {
    wchar_t szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath))) {
        std::wstring localApp = szPath;
        // Check standard Python installation paths for versions 3.8 to 3.15
        for (int v = 15; v >= 8; --v) {
            std::wstring testPath = localApp + L"\\Programs\\Python\\Python3" + std::to_wstring(v) + L"\\python.exe";
            if (PathFileExistsW(testPath.c_str())) {
                return testPath;
            }
        }
    }
    // Fallback: Search the system PATH
    wchar_t szSearchPath[MAX_PATH];
    DWORD dwLen = SearchPathW(NULL, L"python.exe", NULL, MAX_PATH, szSearchPath, NULL);
    if (dwLen > 0 && dwLen < MAX_PATH) {
        return szSearchPath;
    }
    return L"python.exe";
}

// Helper to dynamically locate the process_audio.py script
std::wstring LocatePythonScript() {
    wchar_t szExePath[MAX_PATH];
    GetModuleFileNameW(NULL, szExePath, MAX_PATH);
    PathRemoveFileSpecW(szExePath); // Folder of executable
    
    // Check 1: Next to executable (e.g. build/Release/python/process_audio.py)
    std::wstring path1 = std::wstring(szExePath) + L"\\python\\process_audio.py";
    if (PathFileExistsW(path1.c_str())) return path1;
    
    // Check 2: One level up (e.g. build/python/process_audio.py)
    std::wstring path2 = std::wstring(szExePath) + L"\\..\\python\\process_audio.py";
    if (PathFileExistsW(path2.c_str())) return path2;
    
    // Check 3: Two levels up (e.g. build/Release/../../python/process_audio.py -> project root)
    std::wstring path3 = std::wstring(szExePath) + L"\\..\\..\\python\\process_audio.py";
    if (PathFileExistsW(path3.c_str())) return path3;
    
    // Fallback
    return L"python\\process_audio.py";
}

// Background thread processing Python subprocess call
void EnhanceAudioThread(std::wstring inputPath, std::wstring apiKey) {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    
    std::wstring origFilename = inputPath;
    size_t slash = origFilename.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        origFilename = origFilename.substr(slash + 1);
    }
    
    std::wstring cleanPath = std::wstring(tempPath) + L"enhanced_" + origFilename;
    g_cleanFilePath = cleanPath;

    // Delete existing output if it exists
    DeleteFileW(cleanPath.c_str());

    std::wstring pythonExe = LocatePythonExecutable();
    std::wstring scriptPath = LocatePythonScript();

    // Construct cmd command line, only quote pythonExe if it has spaces
    std::wstring cmdLine = (pythonExe.find(L" ") != std::wstring::npos) ? L"\"" + pythonExe + L"\"" : pythonExe;
    cmdLine += L" \"" + scriptPath + L"\" --input \"" + inputPath + 
                           L"\" --output \"" + cleanPath + 
                           L"\" --api-key \"" + apiKey + L"\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(L'\0');

    BOOL success = CreateProcessW(
        NULL,
        cmdBuffer.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (success) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode == 0) {
            PostMessageW(g_hwndMain, WM_USER_FINISHED, 1, 0);
        } else {
            g_errorMessage = L"API error: Verify that your Nvidia key is correct and that you have an active internet connection. Exit code " + std::to_wstring(exitCode);
            PostMessageW(g_hwndMain, WM_USER_FINISHED, 0, 0);
        }
    } else {
        g_errorMessage = L"Failed to run python.exe process. Check that Python is installed and in your system environment PATH.";
        PostMessageW(g_hwndMain, WM_USER_FINISHED, 0, 0);
    }
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        OleInitialize(NULL);
        DragAcceptFiles(hWnd, TRUE);

        // Edit box for key
        g_hwndEditKey = CreateWindowExW(
            0,
            L"EDIT",
            L"",
            WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD,
            0, 0, 0, 0,
            hWnd,
            NULL,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL
        );

        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessage(g_hwndEditKey, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_apiKey = LoadApiKey();
        if (!g_apiKey.empty()) {
            g_appState = STATE_READY;
            SetWindowTextW(g_hwndEditKey, g_apiKey.c_str());
        } else {
            g_appState = STATE_KEY_INPUT;
        }

        if (FAILED(InitD2D(hWnd))) {
            MessageBoxW(hWnd, L"Graphics engine failure", L"Error", MB_ICONERROR);
            return -1;
        }

        // Run spinner timer at 60fps (16ms) for ultra-smooth animations
        SetTimer(hWnd, TIMER_SPINNER, 16, NULL);
        return 0;
    }
    case WM_TIMER:
        if (wParam == TIMER_SPINNER) {
            // ----- LÓGICA DEL TUMOR PARA FL STUDIO -----
            if (!g_isAttachedToFL) {
                // Buscar la ventana principal de FL Studio (su clase interna es "TFruityLoopsMainForm")
                g_hwndFLStudio = FindWindowW(L"TFruityLoopsMainForm", NULL);
                
                if (g_hwndFLStudio != NULL) {
                    // ¡Encontramos al anfitrión! Nos adherimos a él.
                    // Cambiamos el "Dueño" de nuestra ventana para que sea FL Studio
                    SetWindowLongPtrW(hWnd, GWLP_HWNDPARENT, (LONG_PTR)g_hwndFLStudio);
                    
                    // Aseguramos que se mantenga siempre arriba (Always on Top)
                    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    
                    g_isAttachedToFL = true;
                }
            } else {
                // Si ya estamos adheridos, verificamos si FL Studio sigue vivo
                if (!IsWindow(g_hwndFLStudio)) {
                    // El anfitrión murió (Cerraste FL Studio). Morimos con él.
                    PostQuitMessage(0);
                    return 0;
                }
            }
            // -------------------------------------------

            // Update rotating spinner angles
            g_spinnerAngle += 2.5f;
            if (g_spinnerAngle >= 360.0f) g_spinnerAngle = 0.0f;

            // Scroll grid background
            g_gridOffset += 0.4f;
            if (g_gridOffset >= 40.0f) g_gridOffset = 0.0f;

            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        HWND hwndEdit = (HWND)lParam;
        if (hwndEdit == g_hwndEditKey) {
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, RGB(34, 34, 48)); 
            SetDCBrushColor(hdc, RGB(34, 34, 48));
            return (LRESULT)GetStockObject(DC_BRUSH);
        }
        break;
    }

    case WM_COMMAND: {
        if (HIWORD(wParam) == EN_SETFOCUS) {
            g_isEditFocused = true;
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (HIWORD(wParam) == EN_KILLFOCUS) {
            g_isEditFocused = false;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }

    case WM_SIZE: {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        if (g_pRenderTarget) {
            g_pRenderTarget->Resize(D2D1::SizeU(width, height));
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        if (g_appState == STATE_READY) {
            wchar_t szFilePath[MAX_PATH];
            if (DragQueryFileW(hDrop, 0, szFilePath, MAX_PATH)) {
                g_inputFilePath = szFilePath;
                g_appState = STATE_PROCESSING;
                InvalidateRect(hWnd, NULL, FALSE);
                std::thread(EnhanceAudioThread, g_inputFilePath, g_apiKey).detach();
            }
        }
        DragFinish(hDrop);
        return 0;
    }

    case WM_USER_FINISHED: {
        BOOL success = (BOOL)wParam;
        if (success) {
            g_appState = STATE_COMPLETE;
        } else {
            g_appState = STATE_ERROR;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_SETCURSOR: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        float x = (float)pt.x;
        float y = (float)pt.y;

        bool isHand = false;
        if (g_appState == STATE_KEY_INPUT) {
            if (x >= g_rectBtnSave.left && x <= g_rectBtnSave.right && y >= g_rectBtnSave.top && y <= g_rectBtnSave.bottom) {
                isHand = true;
            }
        } else if (g_appState == STATE_COMPLETE) {
            if (x >= g_rectBtnReset.left && x <= g_rectBtnReset.right && y >= g_rectBtnReset.top && y <= g_rectBtnReset.bottom) {
                isHand = true;
            }
            if (x >= g_rectDragCard.left && x <= g_rectDragCard.right && y >= g_rectDragCard.top && y <= g_rectDragCard.bottom) {
                isHand = true;
            }
        } else if (g_appState == STATE_ERROR) {
            if (x >= g_rectBtnAgain.left && x <= g_rectBtnAgain.right && y >= g_rectBtnAgain.top && y <= g_rectBtnAgain.bottom) {
                isHand = true;
            }
        }

        if (isHand) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        break;
    }

    case WM_MOUSEMOVE: {
        InvalidateRect(hWnd, NULL, FALSE);

        // Drag out
        if (g_appState == STATE_COMPLETE && !g_isDraggingOut) {
            if ((wParam & MK_LBUTTON)) {
                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);
                float x = (float)pt.x;
                float y = (float)pt.y;

                if (x >= g_rectDragCard.left && x <= g_rectDragCard.right && y >= g_rectDragCard.top && y <= g_rectDragCard.bottom) {
                    g_isDraggingOut = true;
                    ReleaseCapture();

                    CDataObject* pDataObject = new CDataObject(g_cleanFilePath.c_str());
                    CDropSource* pDropSource = new CDropSource();
                    DWORD dwEffect = 0;
                    
                    DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY, &dwEffect);
                    
                    pDataObject->Release();
                    pDropSource->Release();

                    g_isDraggingOut = false;
                }
            }
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        float x = (float)pt.x;
        float y = (float)pt.y;

        if (g_appState == STATE_KEY_INPUT) {
            if (x >= g_rectBtnSave.left && x <= g_rectBtnSave.right && y >= g_rectBtnSave.top && y <= g_rectBtnSave.bottom) {
                int len = GetWindowTextLengthW(g_hwndEditKey);
                std::vector<wchar_t> buf(len + 1);
                GetWindowTextW(g_hwndEditKey, buf.data(), len + 1);
                g_apiKey = buf.data();

                if (!g_apiKey.empty()) {
                    SaveApiKey(g_apiKey);
                    g_appState = STATE_READY;
                    InvalidateRect(hWnd, NULL, FALSE);
                } else {
                    MessageBoxW(hWnd, L"API Key cannot be empty.", L"Error", MB_ICONWARNING);
                }
            }
        } else if (g_appState == STATE_COMPLETE) {
            if (x >= g_rectBtnReset.left && x <= g_rectBtnReset.right && y >= g_rectBtnReset.top && y <= g_rectBtnReset.bottom) {
                g_appState = STATE_READY;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        } else if (g_appState == STATE_ERROR) {
            if (x >= g_rectBtnAgain.left && x <= g_rectBtnAgain.right && y >= g_rectBtnAgain.top && y <= g_rectBtnAgain.bottom) {
                g_appState = STATE_READY;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_PAINT:
        Render(hWnd);
        ValidateRect(hWnd, NULL);
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_SPINNER);
        DiscardDeviceResources();
        if (g_pDashedStroke) g_pDashedStroke->Release();
        if (g_pFormatTitle) g_pFormatTitle->Release();
        if (g_pFormatHeader) g_pFormatHeader->Release();
        if (g_pFormatNormal) g_pFormatNormal->Release();
        if (g_pFormatSmall) g_pFormatSmall->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        if (g_pWICFactory) g_pWICFactory->Release();
        
        OleUninitialize();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();

    const wchar_t CLASS_NAME[] = L"NvidiaStudioVoiceWindow";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = NULL; 
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    // Calculate exact square window window dimensions for a 400x400 client area
    RECT rc = { 0, 0, 400, 400 };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);
    int winWidth = rc.right - rc.left;
    int winHeight = rc.bottom - rc.top;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int startX = (screenWidth - winWidth) / 2;
    int startY = (screenHeight - winHeight) / 2;

    g_hwndMain = CreateWindowExW(
        0,
        CLASS_NAME,
        L"CRiSPY",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 
        startX, startY, winWidth, winHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hwndMain == NULL) {
        return 0;
    }

    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
