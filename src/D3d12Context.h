// D3d12Context.h

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <DirectXMath.h>
#include "Snake3D.h" // TODO: Delete this line once generic instances are used

void Init(HWND hwnd);
void InitAssets();
void Update(const DirectX::XMMATRIX& lookAt, float elapsedSeconds);
void Render(const Snake::GamePiece* const* gamePieces, size_t numGamePieces, const DirectX::XMMATRIX& lookAt, float elapsedSeconds); // TODO: Make generic instances with positions, colors, etc.
void Destroy();
void InitTexture(char* dst, uint32_t width, uint32_t height, uint32_t bpp);
