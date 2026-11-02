/*
  File:        Renderer.h
  Description: Direct3D 11 renderer
  Program:     Block Out III

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <vector>
#include "Hud.h"
#include "Menu.h"
#include "Scene.h"
#include "Settings.h"

namespace bo3 {

class Renderer {

    template<class T> using Com = Microsoft::WRL::ComPtr<T>;

  public:

    ~Renderer() { Release(); }
    void Shutdown() { Release(); }

    // Creates the device and the swap chain; false when Direct3D 11 is unavailable
    bool Init(HWND wnd);

    // Window client area changed
    void Resize();

    // Signalled when a new frame can be rendered (VSync pacing)
    HANDLE FrameWaitable() const { return waitable; }

    // Draws and presents one frame of the menu (when menu is set) or of the
    // game. Recovers from a lost device (driver reset/update); returns false
    // only when the device cannot be recreated.
    bool Render(const GameLogic *game,const Menu *menu,const Settings &settings);

  private:

    bool CreateDevice();
    bool CreateSizeDependent();
    bool CreateFontAtlas();
    void DrawUi();
    void DrawMenu(const Menu &menu,const Settings &settings);
    bool UploadVertices();
    bool BeginScene(const D3D11_VIEWPORT &vp,const Matrix &proj,int transparency);
    void PostProcess(const Settings &settings);

    // Full screen pass of the effect shaders
    struct Target {
      Com<ID3D11Texture2D>          tex;
      Com<ID3D11RenderTargetView>   rtv;
      Com<ID3D11ShaderResourceView> srv;
      UINT w = 0, h = 0;
    };
    struct PostConstants {
      float    srcSize[4];
      float    outSize[4];
      float    params[4];
      uint32_t flags[4];
    };
    bool CreateTarget(Target &t,UINT w,UINT h);
    void Pass(ID3D11PixelShader *ps,ID3D11ShaderResourceView *src0,ID3D11ShaderResourceView *src1,
              ID3D11RenderTargetView *rtv,const D3D11_VIEWPORT &vp,const PostConstants &pc);
    void LowRes();
    void Release();
    void Draw(const GameLogic &game,const Settings &settings);
    void DrawRange(size_t first,size_t count,const Matrix &modelView);


    HWND wnd = nullptr;
    HANDLE waitable = nullptr;
    Com<ID3D11Device>           device;
    Com<ID3D11DeviceContext>    ctx;
    Com<IDXGISwapChain2>        swapChain;
    Com<ID3D11RenderTargetView> backBufferRTV;

    // 4:3 canvas: multisampled colour + depth, resolved into canvasTex
    UINT  canvasW = 0, canvasH = 0, outW = 0, outH = 0, samples = 1;
    Com<ID3D11RenderTargetView>   canvasRTV;
    Com<ID3D11DepthStencilView>   canvasDSV;
    Com<ID3D11Texture2D>          canvasMS, canvasTex;
    Com<ID3D11ShaderResourceView> canvasSRV;

    // Pipeline
    Com<ID3D11VertexShader> sceneVS, postVS;
    Com<ID3D11PixelShader>  scenePS;

    // Screen effects: afterglow history (ping-pong), glow at quarter size,
    // 320x200 picture
    Com<ID3D11PixelShader>  persistPS, brightPS, blurPS, lowDownPS, lowUpPS, finalPS;
    Com<ID3D11Buffer>       postCB;
    Target                  persist[2], glow[2], lowres;
    int                     persistCur = 0;
    bool                    persistValid = false;
    LARGE_INTEGER           lastPost{};
    Com<ID3D11InputLayout>  layout;
    Com<ID3D11Buffer>       frameCB, objectCB, vertexBuffer;
    size_t                  vertexCapacity = 0;
    Com<ID3D11RasterizerState>   cullBack, cullFront, cullNone;
    Com<ID3D11DepthStencilState> depthOff, depthLess;
    Com<ID3D11BlendState>        blendOff, blendAlpha;
    Com<ID3D11SamplerState>      linearClamp, pointClamp;

    // 2D overlay
    Com<ID3D11VertexShader>       uiVS;
    Com<ID3D11PixelShader>        uiPS;
    Com<ID3D11InputLayout>        uiLayout;
    Com<ID3D11Buffer>             uiCB, uiBuffer;
    size_t                        uiCapacity = 0;
    Com<ID3D11ShaderResourceView> fontSRV;
    FontInfo                      font{};
    std::vector<UiVertex>         uiVertices;

    std::vector<Vertex> vertices;
    std::vector<MenuDraw> menuDraws;

};

} // namespace bo3
