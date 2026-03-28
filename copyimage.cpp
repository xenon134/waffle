#include <windows.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

using namespace Gdiplus;

// Helper to convert standard std::string to std::wstring (UTF-16)
std::wstring ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Helper to find the GDI+ encoder CLSID for a specific format (e.g., "image/png")
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_image>" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    std::wstring widePath = ToWide(filePath);

    // Initialize COM (Required for Streams)
    CoInitialize(NULL);

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    std::cout << "Processing: " << filePath << std::endl;

    // --- 1. Prepare Plain Text (CF_UNICODETEXT) ---
    size_t textBytes = (widePath.length() + 1) * sizeof(wchar_t);
    HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, textBytes);
    if (hText) {
        memcpy(GlobalLock(hText), widePath.c_str(), textBytes);
        GlobalUnlock(hText);
    }

    // --- 2. Prepare File Drop (CF_HDROP) ---
    // Requires DROPFILES struct + wide string + double null terminator
    size_t dropBytes = sizeof(DROPFILES) + (widePath.length() + 2) * sizeof(wchar_t);
    HGLOBAL hDrop = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dropBytes);
    if (hDrop) {
        DROPFILES* pDrop = (DROPFILES*)GlobalLock(hDrop);
        pDrop->pFiles = sizeof(DROPFILES);
        pDrop->fWide = TRUE;
        wchar_t* pDropFiles = (wchar_t*)((char*)pDrop + sizeof(DROPFILES));
        wcscpy(pDropFiles, widePath.c_str());
        GlobalUnlock(hDrop);
    }

    // --- 3. Convert to PNG bytes using GDI+ ---
    HGLOBAL hImage = NULL;
    Image* image = new Image(widePath.c_str());
    
    if (image->GetLastStatus() == Ok) {
        CLSID pngClsid;
        if (GetEncoderClsid(L"image/png", &pngClsid) != -1) {
            
            // Create a COM stream to hold the encoded PNG bytes in memory
            IStream* pStream = NULL;
            if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) == S_OK) {
                
                // Save the image into the stream as a PNG
                image->Save(pStream, &pngClsid, NULL);
                
                // Get the size of the stream
                STATSTG statstg;
                pStream->Stat(&statstg, STATFLAG_NONAME);
                SIZE_T streamSize = statstg.cbSize.QuadPart;
                
                // Allocate global memory for the clipboard
                hImage = GlobalAlloc(GMEM_MOVEABLE, streamSize);
                if (hImage) {
                    void* pLocked = GlobalLock(hImage);
                    
                    // Reset stream pointer to the beginning and read into our memory
                    LARGE_INTEGER liZero = {};
                    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
                    
                    ULONG bytesRead;
                    pStream->Read(pLocked, streamSize, &bytesRead);
                    GlobalUnlock(hImage);
                }
                pStream->Release();
            }
        }
    } else {
        std::cerr << "Failed to load image via GDI+. Ensure the path is correct." << std::endl;
    }
    delete image;

    // --- 4. Write to Clipboard ---
    if (OpenClipboard(NULL)) {
        EmptyClipboard();

        if (hText) SetClipboardData(CF_UNICODETEXT, hText);
        if (hDrop) SetClipboardData(CF_HDROP, hDrop);
        
        if (hImage) {
            UINT pngFormatId = RegisterClipboardFormatA("PNG");
            SetClipboardData(pngFormatId, hImage);
        }

        CloseClipboard();
        std::cout << "Successfully copied text, file drop, and PNG data to clipboard!" << std::endl;
    } else {
        std::cerr << "Failed to open clipboard." << std::endl;
        // Free memory if clipboard failed (SetClipboardData usually takes ownership)
        if (hText) GlobalFree(hText);
        if (hDrop) GlobalFree(hDrop);
        if (hImage) GlobalFree(hImage);
    }

    // Cleanup
    GdiplusShutdown(gdiplusToken);
    CoUninitialize();

    return 0;
}