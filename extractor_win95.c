#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Windows 95対応のため ANSI版APIを使用 */
#define DEST_DIR "C:\\SAYONARA"

/* BMPとJPEGのヘッダ探索用 */
void ExtractImages(char *filepath, char *filename) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return;

    /* ファイルサイズ取得 */
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* メモリ確保 (Win95のメモリ制約を考慮しエラーチェック) */
    unsigned char *buffer = (unsigned char *)malloc(filesize);
    if (!buffer) {
        printf("Skipped (Out of Memory): %s\n", filename);
        fclose(fp);
        return;
    }

    /* 一括読み込み */
    fread(buffer, 1, filesize, fp);
    fclose(fp);

    int count = 0;
    long i = 0;
    char outpath[MAX_PATH];

    printf("Scanning: %s ...\n", filename);

    while (i < filesize - 6) {
        /* BMP (BM) 探索 */
        if (buffer[i] == 'B' && buffer[i+1] == 'M') {
            /* サイズ情報を読む (リトルエンディアン) */
            unsigned long bmpSize = *(unsigned long *)&buffer[i + 2];
            
            /* 妥当性チェック: 54バイト以上、10MB以下、ファイル終端を超えない */
            if (bmpSize > 54 && bmpSize < 10000000 && (i + bmpSize) <= filesize) {
                /* BMP情報ヘッダサイズチェック (40=Windows3.1/95) */
                unsigned long headerSize = *(unsigned long *)&buffer[i + 14];
                if (headerSize == 40 || headerSize == 12) {
                    sprintf(outpath, "%s\\%s_%04d.bmp", DEST_DIR, filename, count++);
                    FILE *out = fopen(outpath, "wb");
                    if (out) {
                        fwrite(&buffer[i], 1, bmpSize, out);
                        fclose(out);
                        i += bmpSize - 1; /* 画像分進める */
                    }
                }
            }
        }
        /* JPEG (FF D8) 探索 */
        else if (buffer[i] == 0xFF && buffer[i+1] == 0xD8) {
            long start = i;
            long end = -1;
            /* EOI (FF D9) を探す */
            long j = start + 2;
            while (j < filesize - 1) {
                if (buffer[j] == 0xFF && buffer[j+1] == 0xD9) {
                    end = j + 2;
                    break;
                }
                j++;
            }

            if (end != -1) {
                long jpgSize = end - start;
                if (jpgSize > 100) { /* 極小ファイル無視 */
                    sprintf(outpath, "%s\\%s_%04d.jpg", DEST_DIR, filename, count++);
                    FILE *out = fopen(outpath, "wb");
                    if (out) {
                        fwrite(&buffer[start], 1, jpgSize, out);
                        fclose(out);
                        i = end - 1;
                    }
                }
            }
        }
        i++;
    }

    free(buffer);
    if (count > 0) {
        printf(" -> Extracted %d images.\n", count);
    }
}

int main() {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char searchPath[MAX_PATH];

    printf("=== SAYONARA EXTRACTOR FOR WINDOWS 95 ===\n");
    
    /* 保存先ディレクトリ作成 (CreateDirectoryはWin95対応) */
    if (!CreateDirectory(DEST_DIR, NULL)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            printf("Error: Cannot create C:\\SAYONARA\n");
        }
    }
    printf("Output Dir: %s\n", DEST_DIR);

    /* カレントディレクトリの全ファイルを検索 */
    GetCurrentDirectory(MAX_PATH, searchPath);
    strcat(searchPath, "\\*.*");

    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("File not found.\n");
        return 1;
    } 

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char *fname = findFileData.cFileName;
            /* 拡張子簡易チェック */
            int len = strlen(fname);
            if (len > 4) {
                /* 特定の拡張子、または大きなファイルを対象にする */
                /* ここでは全ファイルをスキャン対象とする */
                ExtractImages(fname, fname);
            }
        }
    } while (FindNextFile(hFind, &findFileData));

    FindClose(hFind);
    
    printf("Done.\n");
    /* コンソールがすぐ閉じないように一時停止 */
    system("PAUSE"); 
    return 0;
}
