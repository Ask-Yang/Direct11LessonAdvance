#ifndef D3DAPP_H
#define D3DAPP_H

#include <wrl/client.h>
#include <string>
#include <d2d1.h>
#include <dwrite.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include "Mouse.h"
#include "Keyboard.h"
#include "GameTimer.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "winmm.lib")


template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class D3DApp {
public:
	D3DApp(HINSTANCE hInstance);   
	virtual ~D3DApp();

	HINSTANCE AppInst()const;       
	HWND      MainWnd()const;       
	float     AspectRatio()const;   

	int Run();                      

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	bool InitMainWindow();
	bool InitDirect3D();
	bool InitDirect2D();

private:
	// OnResize Member Func
	void CheckResouceAndRelease();
	void ResetSwapChainAndBuildNewTargetView();
	void AppBuildDepthStencil();
	void SetAppViewPortChange();

	// InitMainWindow Member Func
	void InitMouseAndKeyboard();
	bool BuildWndClass(WNDCLASS& wc);
	bool AdjustWindow(int& width, int& height);
	bool AppCreateWindow(int width, int height);

	// InitDirect3D Member Func
	bool AppCreateD3D11Device(HRESULT& hr);
	bool D3D11DeviceCheck(D3D_FEATURE_LEVEL& featureLevel);
	bool AppCreateD3D11SwapChain(HRESULT& hr);
	bool AppCreateD3D11_1SwapChain(ComPtr<IDXGIFactory2> dxgiFactory2);
	bool AppCreateD3D11_0SwapChain(ComPtr<IDXGIFactory1> dxgiFactory1);

	void CalculateFrameStats(); 

protected:
	DXGI_FORMAT AppDxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

	HINSTANCE m_hAppInst;        
	HWND      m_hMainWnd;        
	bool      m_AppPaused;       
	bool      m_Minimized;       
	bool      m_Maximized;       
	bool      m_Resizing;        
	bool	  m_Enable4xMsaa;	 
	UINT      m_4xMsaaQuality;   

	GameTimer m_Timer;           

	ComPtr<ID2D1Factory> m_pd2dFactory;							
	ComPtr<ID2D1RenderTarget> m_pd2dRenderTarget;				
	ComPtr<IDWriteFactory> m_pdwriteFactory;					
	
	ComPtr<ID3D11Device> m_pd3dDevice;							
	ComPtr<ID3D11DeviceContext> m_pd3dImmediateContext;			
	ComPtr<IDXGISwapChain> m_pSwapChain;						
	
	ComPtr<ID3D11Device1> m_pd3dDevice1;						
	ComPtr<ID3D11DeviceContext1> m_pd3dImmediateContext1;		
	ComPtr<IDXGISwapChain1> m_pSwapChain1;						
	
	ComPtr<ID3D11Texture2D> m_pDepthStencilBuffer;				
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;			
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;			
	D3D11_VIEWPORT m_ScreenViewport;                            
	
	std::unique_ptr<DirectX::Mouse> m_pMouse;					
	DirectX::Mouse::ButtonStateTracker m_MouseTracker;			
	std::unique_ptr<DirectX::Keyboard> m_pKeyboard;				
	DirectX::Keyboard::KeyboardStateTracker m_KeyboardTracker;	
	
	std::wstring m_MainWndCaption;                               
	int m_ClientWidth;                                           
	int m_ClientHeight;                                          
};


class Graphic {
public:
	Graphic(ComPtr<ID3D11DeviceContext> pd3dImmediateContext, ComPtr<ID3D11Device> pd3dDevice);
	Graphic(const Graphic& g);
	~Graphic();
	ComPtr<ID3D11Device> GetDevice();
	ComPtr<ID3D11DeviceContext> GetContext();

private:
	ComPtr<ID3D11Device> m_pd3dDevice;
	ComPtr<ID3D11DeviceContext> m_pd3dImmediateContext;
};


#endif