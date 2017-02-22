/**
 * This file is part of Tales of Berseria "Fix".
 *
 * Tales of Berseria "Fix" is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Tales of Berseria "Fix" is distributed in the hope that it will be
 * useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tales of Berseria "Fix".
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define NOMINMAX

#include <Windows.h>
#include <Wininet.h>
#pragma comment (lib, "wininet.lib")

#include <cstdint>

#include <imgui/imgui.h>

struct tbf_download_s
{
           char  szURL       [INTERNET_MAX_URL_LENGTH] = { '\0' };
           char  szLocalPath [MAX_PATH]                = { '\0' };
  volatile ULONG size          =  0UL;
  volatile ULONG fetched       =  0UL;
  volatile ULONG timeStarted   =    0;
           bool  dialog_active = true;
};

void
TBF_DownloadDialog (tbf_download_s* download)
{
  if (ImGui::BeginPopupModal("Download DLC", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders))
  {
    if (InterlockedExchangeAdd (&download->fetched, 0) == InterlockedExchangeAdd (&download->size, 0))
    {
      if (ImGui::Button ("Finish"))
        download->dialog_active = false;
    }

    else
    {
      if (InterlockedExchangeAdd (&download->size, 0) == 0)
      {
        ImGui::Text       ("Initiating Download From:");
        ImGui::BulletText ("%s", download->szURL);
      }

      else
      {
        ImGui::Text ( "%lu of %lu bytes downloaded...",
                        InterlockedExchangeAdd (&download->fetched, 0),
                          InterlockedExchangeAdd (&download->size, 0) );
      }

      if (ImGui::Button ("Cancel"))
        download->dialog_active = false;
    }

    ImGui::ProgressBar ((float)InterlockedExchangeAdd (&download->fetched, 0UL)/(float)download->size /*, ImVec2 (-1, 0), download->wszLocalPath*/);
  }
}

DWORD
TBF_DownloadThread (LPVOID user)
{
  tbf_download_s* download =
    (tbf_download_s *)user;

  HINTERNET hInetRoot =
    InternetOpenA (
      "Tales of Berseria DLC Installer",
        INTERNET_OPEN_TYPE_DIRECT,
          nullptr, nullptr,
            0x00
    );

  if (! hInetRoot)
    goto CLEANUP;

  DWORD dwInetCtx;

  URL_COMPONENTSA url_comp;
  url_comp.dwStructSize = sizeof URL_COMPONENTSA;

  InternetCrackUrlA (download->szURL, (DWORD)strlen (download->szURL), 0x00, &url_comp);

  HINTERNET hInetHost =
    InternetConnectA ( hInetRoot,
                         url_comp.lpszHostName,
                           INTERNET_DEFAULT_HTTP_PORT,
                             nullptr, nullptr,
                               INTERNET_SERVICE_HTTP,
                                 0x00,
                                   (DWORD_PTR)&dwInetCtx );

  if (! hInetHost) {
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  LPCSTR rgpszAcceptTypes [] = { "*/*", nullptr };

  HINTERNET hInetHTTPGetReq =
    HttpOpenRequestA ( hInetHost,
                         nullptr,
                           url_comp.lpszUrlPath,
                             "HTTP/1.1",
                               nullptr,
                                 rgpszAcceptTypes,
                                                                    INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                  INTERNET_FLAG_CACHE_IF_NET_FAIL | INTERNET_FLAG_IGNORE_CERT_CN_INVALID   |
                                  INTERNET_FLAG_RESYNCHRONIZE     | INTERNET_FLAG_CACHE_ASYNC,
                                    (DWORD_PTR)&dwInetCtx );

  if (! hInetHTTPGetReq) {
    InternetCloseHandle (hInetHost);
    InternetCloseHandle (hInetRoot);
    goto CLEANUP;
  }

  if ( HttpSendRequestW ( hInetHTTPGetReq,
                            nullptr,
                              0,
                                nullptr,
                                  0 ) ) {

    DWORD dwContentLength     = 0;
    DWORD dwContentLength_Len = sizeof DWORD;
    DWORD dwSize;

    HttpQueryInfo ( hInetHTTPGetReq,
                      HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                        &dwContentLength,
                          &dwContentLength_Len,
                            nullptr );

    InterlockedExchange (&download->size, dwContentLength);

    DWORD dwTotalBytesDownloaded = 0UL;

    if ( InternetQueryDataAvailable ( hInetHTTPGetReq,
                                        &dwSize,
                                          0x00, NULL )
      )
    {
      HANDLE hGetFile =
        CreateFileA ( download->szLocalPath,
                        GENERIC_WRITE,
                          FILE_SHARE_READ,
                            nullptr,
                              CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                  nullptr );

      while (hGetFile != INVALID_HANDLE_VALUE && dwSize > 0) {
        DWORD    dwRead = 0;
        uint8_t *pData  = new uint8_t [dwSize];

        if (! pData)
          break;

        if ( InternetReadFile ( hInetHTTPGetReq,
                                  pData,
                                    dwSize,
                                      &dwRead )
          )
        {
          DWORD dwWritten;

          WriteFile ( hGetFile,
                        pData,
                          dwRead,
                            &dwWritten,
                              nullptr );

          dwTotalBytesDownloaded += dwRead;

          InterlockedExchangeAdd (&download->fetched, dwRead);
        }

        delete [] pData;
        pData = nullptr;

        if (! InternetQueryDataAvailable ( hInetHTTPGetReq,
                                             &dwSize,
                                               0x00, NULL
                                         )
           ) break;
      }

      if (hGetFile != INVALID_HANDLE_VALUE)
        CloseHandle (hGetFile);

      //else
        //SetErrorState ();
    }
  }

  else {
    //SetErrorState ();
  }

  InternetCloseHandle (hInetHTTPGetReq);
  InternetCloseHandle (hInetHost);
  InternetCloseHandle (hInetRoot);

  goto END;

CLEANUP:
  //SetErrorState ();
  //get->status = STATUS_FAILED;

END:

  CloseHandle (GetCurrentThread ());

  return 0;
}