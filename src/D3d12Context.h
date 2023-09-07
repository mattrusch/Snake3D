// D3d12Context.h

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <DirectXMath.h>

void Init(HWND hwnd);
void InitAssets();
void Update(const DirectX::XMMATRIX& lookAt, float elapsedSeconds);
void Render();
void Destroy();
void InitTexture(char* dst, uint32_t width, uint32_t height, uint32_t bpp);
