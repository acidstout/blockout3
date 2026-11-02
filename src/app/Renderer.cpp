/*
  File:        Renderer.cpp
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
#include "Renderer.h"
#include "Font8x8.h"
#include <algorithm>
#include <cstring>
#include "SceneVS.h"
#include "ScenePS.h"
#include "PostVS.h"
#include "PostPersistPS.h"
#include "PostBrightPS.h"
#include "PostBlurPS.h"
#include "PostLowDownPS.h"
#include "PostLowUpPS.h"
#include "PostFinalPS.h"
#include <cmath>
#include "UiVS.h"
#include "UiPS.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dxgi.lib")

namespace bo3 {

namespace {

struct FrameConstants {
  float    proj[16];
  float    viewport[4];
  float    lightEye[4];
  Material mats[MAT_COUNT];
};

// Light 0 of BlockOut II: (15,10,-10) given while the game view was loaded
const float LIGHT_EYE[4] = { -15.0f, 10.0f, 10.0f, 1.0f };

// BlockOut II pit viewport, in fractions of the 4:3 screen
const float PIT_X = 0.0889f, PIT_Y = 0.0183f, PIT_W = 0.7197f, PIT_H = 0.9596f;

}

//-----------------------------------------------------------------------------

bool Renderer::Init(HWND w) {
  wnd = w;
  return CreateDevice() && CreateSizeDependent();
}

void Renderer::Release() {

  if( ctx ) ctx->ClearState();
  if( waitable ) { CloseHandle(waitable); waitable = nullptr; }
  canvasRTV.Reset(); canvasDSV.Reset(); canvasMS.Reset(); canvasTex.Reset(); canvasSRV.Reset();
  backBufferRTV.Reset(); swapChain.Reset();
  sceneVS.Reset(); postVS.Reset(); scenePS.Reset(); layout.Reset();
  persistPS.Reset(); brightPS.Reset(); blurPS.Reset(); lowDownPS.Reset(); lowUpPS.Reset(); finalPS.Reset(); postCB.Reset();
  for(Target *t : { &persist[0], &persist[1], &glow[0], &glow[1], &lowres }) *t = Target();
  persistValid = false;
  frameCB.Reset(); objectCB.Reset(); vertexBuffer.Reset(); vertexCapacity = 0;
  cullBack.Reset(); cullFront.Reset(); cullNone.Reset(); depthOff.Reset(); depthLess.Reset();
  blendOff.Reset(); blendAlpha.Reset(); linearClamp.Reset(); pointClamp.Reset();
  uiVS.Reset(); uiPS.Reset(); uiLayout.Reset(); uiCB.Reset(); uiBuffer.Reset(); uiCapacity = 0; fontSRV.Reset();
  ctx.Reset(); device.Reset();
  canvasW = canvasH = outW = outH = 0;

}

//-----------------------------------------------------------------------------

bool Renderer::CreateDevice() {

  UINT flags = 0;
#ifdef _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
  HRESULT hr = D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,levels,3,
                                 D3D11_SDK_VERSION,&device,nullptr,&ctx);
  if( FAILED(hr) && flags ) {   // debug layer not installed (optional Windows feature)
    flags = 0;
    hr = D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,levels,3,
                           D3D11_SDK_VERSION,&device,nullptr,&ctx);
  }
  if( FAILED(hr) )   // no usable GPU driver: software rasterizer
    hr = D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,flags,levels,3,
                           D3D11_SDK_VERSION,&device,nullptr,&ctx);
  if( FAILED(hr) ) return false;

  // Swap chain: flip model, VSync pacing through the frame latency waitable
  Com<IDXGIDevice> dxgiDevice;
  Com<IDXGIAdapter> adapter;
  Com<IDXGIFactory2> factory;
  if( FAILED(device.As(&dxgiDevice)) || FAILED(dxgiDevice->GetAdapter(&adapter)) ||
      FAILED(adapter->GetParent(IID_PPV_ARGS(&factory))) ) return false;

  DXGI_SWAP_CHAIN_DESC1 sd = {};
  sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  sd.SampleDesc.Count = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount = 2;
  sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  Com<IDXGISwapChain1> sc1;
  if( FAILED(factory->CreateSwapChainForHwnd(device.Get(),wnd,&sd,nullptr,nullptr,&sc1)) ||
      FAILED(sc1.As(&swapChain)) ) return false;
  factory->MakeWindowAssociation(wnd,DXGI_MWA_NO_ALT_ENTER);   // Alt+Enter is ours
  swapChain->SetMaximumFrameLatency(1);
  waitable = swapChain->GetFrameLatencyWaitableObject();

  // Shaders and input layout
  if( FAILED(device->CreateVertexShader(g_SceneVS,sizeof(g_SceneVS),nullptr,&sceneVS)) ||
      FAILED(device->CreatePixelShader(g_ScenePS,sizeof(g_ScenePS),nullptr,&scenePS)) ||
      FAILED(device->CreateVertexShader(g_PostVS,sizeof(g_PostVS),nullptr,&postVS)) ||
      FAILED(device->CreatePixelShader(g_PostPersistPS,sizeof(g_PostPersistPS),nullptr,&persistPS)) ||
      FAILED(device->CreatePixelShader(g_PostBrightPS,sizeof(g_PostBrightPS),nullptr,&brightPS)) ||
      FAILED(device->CreatePixelShader(g_PostBlurPS,sizeof(g_PostBlurPS),nullptr,&blurPS)) ||
      FAILED(device->CreatePixelShader(g_PostLowDownPS,sizeof(g_PostLowDownPS),nullptr,&lowDownPS)) ||
      FAILED(device->CreatePixelShader(g_PostLowUpPS,sizeof(g_PostLowUpPS),nullptr,&lowUpPS)) ||
      FAILED(device->CreatePixelShader(g_PostFinalPS,sizeof(g_PostFinalPS),nullptr,&finalPS)) ) return false;
  const D3D11_INPUT_ELEMENT_DESC il[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex,pos),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex,aux),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32_UINT,        0, offsetof(Vertex,info), D3D11_INPUT_PER_VERTEX_DATA, 0 } };
  if( FAILED(device->CreateInputLayout(il,3,g_SceneVS,sizeof(g_SceneVS),&layout)) ) return false;

  // Constant buffers
  D3D11_BUFFER_DESC bd = {};
  bd.Usage = D3D11_USAGE_DYNAMIC;
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  bd.ByteWidth = sizeof(FrameConstants);
  if( FAILED(device->CreateBuffer(&bd,nullptr,&frameCB)) ) return false;
  bd.ByteWidth = 64;
  if( FAILED(device->CreateBuffer(&bd,nullptr,&objectCB)) ) return false;

  // States. GL front faces are counter clockwise.
  D3D11_RASTERIZER_DESC rd = {};
  rd.FillMode = D3D11_FILL_SOLID;
  rd.FrontCounterClockwise = TRUE;
  rd.DepthClipEnable = TRUE;
  rd.MultisampleEnable = TRUE;
  rd.CullMode = D3D11_CULL_BACK;  device->CreateRasterizerState(&rd,&cullBack);
  rd.CullMode = D3D11_CULL_FRONT; device->CreateRasterizerState(&rd,&cullFront);
  rd.CullMode = D3D11_CULL_NONE;  device->CreateRasterizerState(&rd,&cullNone);

  D3D11_DEPTH_STENCIL_DESC dd = {};
  device->CreateDepthStencilState(&dd,&depthOff);
  dd.DepthEnable = TRUE;
  dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  dd.DepthFunc = D3D11_COMPARISON_LESS;
  device->CreateDepthStencilState(&dd,&depthLess);

  D3D11_BLEND_DESC bld = {};
  bld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  device->CreateBlendState(&bld,&blendOff);
  auto &rt = bld.RenderTarget[0];
  rt.BlendEnable = TRUE;
  rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;   rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   rt.BlendOp = D3D11_BLEND_OP_ADD;
  rt.SrcBlendAlpha = D3D11_BLEND_ONE;    rt.DestBlendAlpha = D3D11_BLEND_ZERO;       rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
  device->CreateBlendState(&bld,&blendAlpha);

  D3D11_SAMPLER_DESC smp = {};
  smp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  smp.AddressU = smp.AddressV = smp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  smp.MaxLOD = D3D11_FLOAT32_MAX;
  device->CreateSamplerState(&smp,&linearClamp);
  smp.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  device->CreateSamplerState(&smp,&pointClamp);

  // 2D overlay pipeline
  if( FAILED(device->CreateVertexShader(g_UiVS,sizeof(g_UiVS),nullptr,&uiVS)) ||
      FAILED(device->CreatePixelShader(g_UiPS,sizeof(g_UiPS),nullptr,&uiPS)) ) return false;
  const D3D11_INPUT_ELEMENT_DESC uil[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, offsetof(UiVertex,x),     D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, offsetof(UiVertex,u),     D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(UiVertex,color), D3D11_INPUT_PER_VERTEX_DATA, 0 } };
  if( FAILED(device->CreateInputLayout(uil,3,g_UiVS,sizeof(g_UiVS),&uiLayout)) ) return false;
  bd.ByteWidth = 16;
  if( FAILED(device->CreateBuffer(&bd,nullptr,&uiCB)) || !CreateFontAtlas() ) return false;
  bd.ByteWidth = sizeof(PostConstants);
  if( FAILED(device->CreateBuffer(&bd,nullptr,&postCB)) ) return false;

  return cullBack && cullFront && cullNone && depthOff && depthLess && blendOff && blendAlpha &&
         linearClamp && pointClamp;

}

//-----------------------------------------------------------------------------
// Built-in pixel font (Font8x8.cpp)

bool Renderer::CreateFontAtlas() {

  std::vector<uint8_t> coverage;
  BuildFontAtlas(coverage,font);

  D3D11_TEXTURE2D_DESC td = {};
  td.Width = font.atlasW;
  td.Height = font.atlasH;
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = DXGI_FORMAT_R8_UNORM;
  td.SampleDesc.Count = 1;
  td.Usage = D3D11_USAGE_IMMUTABLE;
  td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA init = { coverage.data(), (UINT)font.atlasW, 0 };
  Com<ID3D11Texture2D> tex;
  return SUCCEEDED(device->CreateTexture2D(&td,&init,&tex)) &&
         SUCCEEDED(device->CreateShaderResourceView(tex.Get(),nullptr,&fontSRV));

}

//-----------------------------------------------------------------------------

void Renderer::DrawUi() {

  if( uiVertices.empty() ) return;

  if( uiVertices.size() > uiCapacity ) {
    uiBuffer.Reset();
    uiCapacity = std::max<size_t>(uiVertices.size()*2,16384);
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (UINT)(uiCapacity*sizeof(UiVertex));
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if( FAILED(device->CreateBuffer(&bd,nullptr,&uiBuffer)) ) { uiCapacity = 0; return; }
  }
  D3D11_MAPPED_SUBRESOURCE map;
  if( FAILED(ctx->Map(uiBuffer.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map)) ) return;
  memcpy(map.pData,uiVertices.data(),uiVertices.size()*sizeof(UiVertex));
  ctx->Unmap(uiBuffer.Get(),0);
  if( FAILED(ctx->Map(uiCB.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map)) ) return;
  const float size[4] = { (float)canvasW, (float)canvasH, 0.0f, 0.0f };
  memcpy(map.pData,size,sizeof(size));
  ctx->Unmap(uiCB.Get(),0);

  D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)canvasW, (float)canvasH, 0.0f, 1.0f };
  ctx->RSSetViewports(1,&vp);
  ctx->RSSetState(cullNone.Get());
  ctx->OMSetDepthStencilState(depthOff.Get(),0);
  ctx->OMSetBlendState(blendAlpha.Get(),nullptr,0xFFFFFFFF);
  UINT stride = sizeof(UiVertex), offset = 0;
  ctx->IASetVertexBuffers(0,1,uiBuffer.GetAddressOf(),&stride,&offset);
  ctx->IASetInputLayout(uiLayout.Get());
  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->VSSetShader(uiVS.Get(),nullptr,0);
  ctx->VSSetConstantBuffers(0,1,uiCB.GetAddressOf());
  ctx->PSSetShader(uiPS.Get(),nullptr,0);
  ctx->PSSetShaderResources(0,1,fontSRV.GetAddressOf());
  ctx->PSSetSamplers(0,1,pointClamp.GetAddressOf());
  ctx->Draw((UINT)uiVertices.size(),0);
  ctx->OMSetBlendState(blendOff.Get(),nullptr,0xFFFFFFFF);

}

//-----------------------------------------------------------------------------

bool Renderer::CreateSizeDependent() {

  Com<ID3D11Texture2D> back;
  if( FAILED(swapChain->GetBuffer(0,IID_PPV_ARGS(&back))) ||
      FAILED(device->CreateRenderTargetView(back.Get(),nullptr,&backBufferRTV)) ) return false;
  D3D11_TEXTURE2D_DESC bbd;
  back->GetDesc(&bbd);
  outW = bbd.Width;
  outH = bbd.Height;

  // Largest 4:3 rectangle that fits the window
  canvasH = outH;
  canvasW = (outH * 4 + 1) / 3;
  if( canvasW > outW ) { canvasW = outW; canvasH = (outW * 3 + 2) / 4; }
  canvasW = std::max(canvasW,1u);
  canvasH = std::max(canvasH,1u);

  // Antialiasing: 4x when available
  samples = 1;
  UINT quality = 0;
  if( SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,4,&quality)) && quality )
    samples = 4;

  D3D11_TEXTURE2D_DESC td = {};
  td.Width = canvasW;
  td.Height = canvasH;
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count = samples;
  td.BindFlags = D3D11_BIND_RENDER_TARGET;
  if( FAILED(device->CreateTexture2D(&td,nullptr,&canvasMS)) ||
      FAILED(device->CreateRenderTargetView(canvasMS.Get(),nullptr,&canvasRTV)) ) return false;

  td.SampleDesc.Count = 1;
  td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  if( FAILED(device->CreateTexture2D(&td,nullptr,&canvasTex)) ||
      FAILED(device->CreateShaderResourceView(canvasTex.Get(),nullptr,&canvasSRV)) ) return false;

  // Effect buffers
  persistValid = false;
  if( !CreateTarget(persist[0],canvasW,canvasH) || !CreateTarget(persist[1],canvasW,canvasH) ||
      !CreateTarget(glow[0],std::max(1u,canvasW/4),std::max(1u,canvasH/4)) ||
      !CreateTarget(glow[1],std::max(1u,canvasW/4),std::max(1u,canvasH/4)) ||
      !CreateTarget(lowres,320,200) ) return false;

  td.Format = DXGI_FORMAT_D32_FLOAT;
  td.SampleDesc.Count = samples;
  td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  Com<ID3D11Texture2D> depth;
  return SUCCEEDED(device->CreateTexture2D(&td,nullptr,&depth)) &&
         SUCCEEDED(device->CreateDepthStencilView(depth.Get(),nullptr,&canvasDSV));

}

//-----------------------------------------------------------------------------

void Renderer::Resize() {

  if( !swapChain ) return;
  RECT rc;
  GetClientRect(wnd,&rc);
  if( rc.right<=0 || rc.bottom<=0 ) return;   // minimized
  if( (UINT)rc.right==outW && (UINT)rc.bottom==outH ) return;

  ctx->OMSetRenderTargets(0,nullptr,nullptr);
  backBufferRTV.Reset(); canvasRTV.Reset(); canvasDSV.Reset();
  canvasMS.Reset(); canvasTex.Reset(); canvasSRV.Reset();
  for(Target *t : { &persist[0], &persist[1], &glow[0], &glow[1], &lowres }) *t = Target();
  ctx->Flush();
  HRESULT hr = swapChain->ResizeBuffers(0,0,0,DXGI_FORMAT_UNKNOWN,DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
  if( FAILED(hr) || !CreateSizeDependent() ) {
    // Lost device during the resize: start over
    Release();
    Init(wnd);
  }

}

//-----------------------------------------------------------------------------

void Renderer::DrawRange(size_t first,size_t count,const Matrix &modelView) {

  if( !count ) return;
  D3D11_MAPPED_SUBRESOURCE map;
  if( FAILED(ctx->Map(objectCB.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map)) ) return;
  memcpy(map.pData,&modelView._11,64);
  ctx->Unmap(objectCB.Get(),0);
  ctx->Draw((UINT)count,(UINT)first);

}

//-----------------------------------------------------------------------------
// Shared steps of the 3D passes

bool Renderer::UploadVertices() {

  if( vertices.size() > vertexCapacity ) {
    vertexBuffer.Reset();
    vertexCapacity = std::max<size_t>(vertices.size()*2,65536);
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (UINT)(vertexCapacity*sizeof(Vertex));
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if( FAILED(device->CreateBuffer(&bd,nullptr,&vertexBuffer)) ) { vertexCapacity = 0; return false; }
  }
  if( vertices.empty() ) return true;
  D3D11_MAPPED_SUBRESOURCE map;
  if( FAILED(ctx->Map(vertexBuffer.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map)) ) return false;
  memcpy(map.pData,vertices.data(),vertices.size()*sizeof(Vertex));
  ctx->Unmap(vertexBuffer.Get(),0);
  return true;

}

// Clears the canvas and sets up the scene pipeline for the given viewport
bool Renderer::BeginScene(const D3D11_VIEWPORT &vp,const Matrix &proj,int transparency) {

  FrameConstants fc = {};
  memcpy(fc.proj,&proj._11,64);
  fc.viewport[0] = vp.Width;
  fc.viewport[1] = vp.Height;
  fc.viewport[2] = std::max(0.5f,(float)canvasH/1536.0f);   // 1 pixel at 768 lines
  memcpy(fc.lightEye,LIGHT_EYE,sizeof(LIGHT_EYE));
  GetMaterials(fc.mats,transparency);
  D3D11_MAPPED_SUBRESOURCE map;
  if( FAILED(ctx->Map(frameCB.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map)) ) return false;
  memcpy(map.pData,&fc,sizeof(fc));
  ctx->Unmap(frameCB.Get(),0);

  const float black[4] = { 0,0,0,1 };
  ctx->OMSetRenderTargets(1,canvasRTV.GetAddressOf(),canvasDSV.Get());
  ctx->ClearRenderTargetView(canvasRTV.Get(),black);
  ctx->ClearDepthStencilView(canvasDSV.Get(),D3D11_CLEAR_DEPTH,1.0f,0);
  ctx->RSSetViewports(1,&vp);
  UINT stride = sizeof(Vertex), offset = 0;
  ctx->IASetVertexBuffers(0,1,vertexBuffer.GetAddressOf(),&stride,&offset);
  ctx->IASetInputLayout(layout.Get());
  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->VSSetShader(sceneVS.Get(),nullptr,0);
  ctx->PSSetShader(scenePS.Get(),nullptr,0);
  ID3D11Buffer *cbs[2] = { frameCB.Get(), objectCB.Get() };
  ctx->VSSetConstantBuffers(0,2,cbs);
  ctx->RSSetState(cullBack.Get());
  ctx->OMSetBlendState(blendOff.Get(),nullptr,0xFFFFFFFF);
  ctx->OMSetDepthStencilState(depthOff.Get(),0);
  return true;

}

bool Renderer::CreateTarget(Target &t,UINT w,UINT h) {

  D3D11_TEXTURE2D_DESC td = {};
  td.Width = w;
  td.Height = h;
  td.MipLevels = 1;
  td.ArraySize = 1;
  td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  td.SampleDesc.Count = 1;
  td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  t.w = w;
  t.h = h;
  return SUCCEEDED(device->CreateTexture2D(&td,nullptr,&t.tex)) &&
         SUCCEEDED(device->CreateRenderTargetView(t.tex.Get(),nullptr,&t.rtv)) &&
         SUCCEEDED(device->CreateShaderResourceView(t.tex.Get(),nullptr,&t.srv));

}

void Renderer::Pass(ID3D11PixelShader *ps,ID3D11ShaderResourceView *src0,ID3D11ShaderResourceView *src1,
                    ID3D11RenderTargetView *rtv,const D3D11_VIEWPORT &vp,const PostConstants &pc) {

  D3D11_MAPPED_SUBRESOURCE map;
  if( FAILED(ctx->Map(postCB.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map)) ) return;
  memcpy(map.pData,&pc,sizeof(pc));
  ctx->Unmap(postCB.Get(),0);

  ctx->OMSetRenderTargets(1,&rtv,nullptr);
  ctx->RSSetViewports(1,&vp);
  ctx->OMSetDepthStencilState(depthOff.Get(),0);
  ctx->OMSetBlendState(blendOff.Get(),nullptr,0xFFFFFFFF);
  ctx->RSSetState(cullNone.Get());
  ctx->IASetInputLayout(nullptr);
  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->VSSetShader(postVS.Get(),nullptr,0);
  ctx->PSSetShader(ps,nullptr,0);
  ctx->PSSetConstantBuffers(0,1,postCB.GetAddressOf());
  ID3D11ShaderResourceView *srv[2] = { src0, src1 };
  ctx->PSSetShaderResources(0,2,srv);
  ID3D11SamplerState *smp[2] = { linearClamp.Get(), pointClamp.Get() };
  ctx->PSSetSamplers(0,2,smp);
  ctx->Draw(3,0);
  ID3D11ShaderResourceView *none[2] = {};
  ctx->PSSetShaderResources(0,2,none);

}

static D3D11_VIEWPORT Viewport(float x,float y,float w,float h) {
  return D3D11_VIEWPORT{ x, y, w, h, 0.0f, 1.0f };
}

// 320x200 mode: the 3D picture is averaged down to 320x200 and drawn back
// with big pixels (texts are drawn afterwards and stay sharp)
void Renderer::LowRes() {

  ctx->ResolveSubresource(canvasTex.Get(),0,canvasMS.Get(),0,DXGI_FORMAT_R8G8B8A8_UNORM);
  PostConstants pc = {};
  pc.srcSize[0] = (float)canvasW; pc.srcSize[1] = (float)canvasH;
  pc.srcSize[2] = 1.0f/canvasW;   pc.srcSize[3] = 1.0f/canvasH;
  Pass(lowDownPS.Get(),canvasSRV.Get(),nullptr,lowres.rtv.Get(),Viewport(0,0,320,200),pc);
  Pass(lowUpPS.Get(),lowres.srv.Get(),nullptr,canvasRTV.Get(),Viewport(0,0,(float)canvasW,(float)canvasH),pc);

}

// Canvas -> window with the screen effects (black bars around the 4:3 picture)
void Renderer::PostProcess(const Settings &settings) {

  const FxSettings &fx = settings.fx;
  ctx->ResolveSubresource(canvasTex.Get(),0,canvasMS.Get(),0,DXGI_FORMAT_R8G8B8A8_UNORM);

  PostConstants pc = {};
  auto source = [&](float w,float h) {
    pc.srcSize[0] = w; pc.srcSize[1] = h; pc.srcSize[2] = 1.0f/w; pc.srcSize[3] = 1.0f/h;
  };
  ID3D11ShaderResourceView *picture = canvasSRV.Get();

  // Afterglow: the previous picture fades with a 45 ms time constant
  LARGE_INTEGER now, freq;
  QueryPerformanceCounter(&now);
  QueryPerformanceFrequency(&freq);
  double dt = lastPost.QuadPart ? (double)(now.QuadPart - lastPost.QuadPart) / (double)freq.QuadPart : 0.0;
  lastPost = now;
  if( fx.afterglow ) {
    pc.params[0] = persistValid ? (float)exp(-std::min(dt,0.1) / 0.045) : 0.0f;
    source((float)canvasW,(float)canvasH);
    int prev = persistCur;
    persistCur ^= 1;
    Pass(persistPS.Get(),canvasSRV.Get(),persist[prev].srv.Get(),persist[persistCur].rtv.Get(),
         Viewport(0,0,(float)canvasW,(float)canvasH),pc);
    picture = persist[persistCur].srv.Get();
    persistValid = true;
  } else {
    persistValid = false;
  }

  // Phosphor glow: bright parts blurred at quarter size
  if( fx.glow ) {
    float gw = (float)glow[0].w, gh = (float)glow[0].h;
    source((float)canvasW,(float)canvasH);
    Pass(brightPS.Get(),picture,nullptr,glow[0].rtv.Get(),Viewport(0,0,gw,gh),pc);
    source(gw,gh);
    pc.params[2] = 1.0f; pc.params[3] = 0.0f;
    Pass(blurPS.Get(),glow[0].srv.Get(),nullptr,glow[1].rtv.Get(),Viewport(0,0,gw,gh),pc);
    pc.params[2] = 0.0f; pc.params[3] = 1.0f;
    Pass(blurPS.Get(),glow[1].srv.Get(),nullptr,glow[0].rtv.Get(),Viewport(0,0,gw,gh),pc);
  }

  // Final pass into the window
  const float black[4] = { 0,0,0,1 };
  ctx->ClearRenderTargetView(backBufferRTV.Get(),black);
  pc = {};
  source((float)canvasW,(float)canvasH);
  pc.outSize[0] = (float)canvasW;
  pc.outSize[1] = (float)canvasH;
  pc.params[1] = fx.lowRes ? 200.0f : 240.0f;   // scanlines over the picture height
  pc.flags[0] = (fx.scanlines ? 1u : 0u) | (fx.vignette ? 2u : 0u) | (fx.glow ? 4u : 0u) |
                (fx.curvature ? 8u : 0u) | (fx.mask ? 16u : 0u) | (fx.ega ? 32u : 0u);
  Pass(finalPS.Get(),picture,fx.glow ? glow[0].srv.Get() : nullptr,backBufferRTV.Get(),
       Viewport((float)((outW-canvasW)/2),(float)((outH-canvasH)/2),(float)canvasW,(float)canvasH),pc);

}

//-----------------------------------------------------------------------------

void Renderer::DrawMenu(const Menu &menu,const Settings &settings) {

  vertices.clear();
  menuDraws.clear();
  menu.Build3D(vertices,menuDraws);
  if( !UploadVertices() ) return;

  D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)canvasW, (float)canvasH, 0.0f, 1.0f };
  if( !BeginScene(vp,Perspective(60.0f,(float)canvasW/(float)canvasH,0.1f,10.0f),settings.transparentFace) ) return;
  for(const MenuDraw &d : menuDraws) DrawRange(d.first,d.count,d.modelView);

  if( settings.fx.lowRes ) LowRes();
  uiVertices.clear();
  menu.BuildUi((int)canvasW,(int)canvasH,font,uiVertices);
  DrawUi();
  PostProcess(settings);

}

//-----------------------------------------------------------------------------

void Renderer::Draw(const GameLogic &game,const Settings &settings) {

  const Pit &pit = game.GetPit();
  PitGeometry g = MakePitGeometry(pit.GetWidth(),pit.GetHeight(),pit.GetDepth());
  int mode = game.GetMode();
  bool endAnim = game.IsEndAnimStarted();
  bool blockVisible = mode!=GAME_PAUSED && mode!=GAME_OVER;
  float t = game.GetCurrentTime();
  bool help = game.IsPractice() && game.GetHelpTime()>0.0f && t-game.GetHelpTime()<=0.75f;
  const PolyCube &block = game.GetPolyCube(game.GetPolyCubeIdx());
  bool bigEdges = settings.lineWidth>0;

  // Geometry of this frame, drawn in this order
  vertices.clear();
  BuildPit(pit,g,mode!=GAME_PAUSED,endAnim,vertices);
  size_t nPit = vertices.size();
  if( help ) BuildBlockLines(block,g,MAT_RED,vertices);
  size_t nHelp = vertices.size();
  if( blockVisible && settings.transparentFace>0 ) BuildBlockGhost(block,g,vertices);
  size_t nGhost = vertices.size();
  MaterialId edgeMat = game.IsRedMode() ? MAT_RED : (bigEdges ? MAT_GRAY : MAT_WHITE);
  if( blockVisible ) {
    if( bigEdges ) BuildBlockBigEdges(block,g,(float)settings.lineWidth*0.002f+0.005f,edgeMat,vertices);
    else           BuildBlockLines(block,g,edgeMat,vertices);
  }
  size_t nBlock = vertices.size();

  if( !UploadVertices() ) return;

  // Pit viewport (square) inside the canvas
  D3D11_VIEWPORT vp = {};
  vp.TopLeftX = (float)fround(canvasW*PIT_X);
  vp.TopLeftY = (float)fround(canvasH*PIT_Y);
  vp.Width    = (float)fround(canvasW*PIT_W);
  vp.Height   = (float)fround(canvasH*PIT_H);
  vp.MaxDepth = 1.0f;

  if( !BeginScene(vp,Perspective(60.0f,1.0f,0.1f,10.0f),settings.transparentFace) ) return;

  // Pit: painter's algorithm, or the depth buffer during the game over animation
  Matrix view = GameView();
  Matrix pitView = view;
  if( endAnim ) {
    EndCamera cam = game.GetEndCamera();
    pitView = LookAt(cam.eye,cam.target,cam.up);
  }
  ctx->OMSetDepthStencilState(endAnim ? depthLess.Get() : depthOff.Get(),0);
  DrawRange(0,nPit,pitView);

  // Practice: suggested position
  ctx->RSSetState(cullNone.Get());
  ctx->OMSetDepthStencilState(depthOff.Get(),0);
  if( help ) DrawRange(nPit,nHelp-nPit,HelpModelView(game,g,view));

  // Falling block: transparent faces (back then front), then the outline
  if( blockVisible ) {
    Matrix mv = BlockModelView(game,g,view);
    ctx->OMSetBlendState(blendAlpha.Get(),nullptr,0xFFFFFFFF);
    ctx->RSSetState(cullFront.Get());
    DrawRange(nHelp,nGhost-nHelp,mv);
    ctx->RSSetState(cullBack.Get());
    DrawRange(nHelp,nGhost-nHelp,mv);
    ctx->OMSetBlendState(blendOff.Get(),nullptr,0xFFFFFFFF);
    ctx->RSSetState(cullNone.Get());
    if( bigEdges ) {
      ctx->ClearDepthStencilView(canvasDSV.Get(),D3D11_CLEAR_DEPTH,1.0f,0);
      ctx->OMSetDepthStencilState(depthLess.Get(),0);
    }
    DrawRange(nGhost,nBlock-nGhost,mv);
  }

  if( settings.fx.lowRes ) LowRes();
  uiVertices.clear();
  BuildHud(game,(int)canvasW,(int)canvasH,font,uiVertices);
  DrawUi();
  PostProcess(settings);

}

//-----------------------------------------------------------------------------

bool Renderer::Render(const GameLogic *game,const Menu *menu,const Settings &settings) {

  if( !swapChain && !Init(wnd) ) return false;

  if( menu ) DrawMenu(*menu,settings);
  else       Draw(*game,settings);
  HRESULT hr = swapChain->Present(1,0);
  if( hr==DXGI_ERROR_DEVICE_REMOVED || hr==DXGI_ERROR_DEVICE_RESET ) {
    // Driver update or GPU reset: recreate everything
    Release();
    return Init(wnd);
  }
  return true;

}

} // namespace bo3
