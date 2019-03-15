/*
 *
 *	ChartCtrl.cpp
 *
 *	Written by C閐ric Moonen (cedric_moonen@hotmail.com)
 *
 *
 *
 *	This code may be used for any non-commercial and commercial purposes in a compiled form.
 *	The code may be redistributed as long as it remains unmodified and providing that the 
 *	author name and this disclaimer remain intact. The sources can be modified WITH the author 
 *	consent only.
 *	
 *	This code is provided without any garanties. I cannot be held responsible for the damage or
 *	the loss of time it causes. Use it at your own risks
 *
 *	An e-mail to notify me that you are using this code is appreciated also.
 *
 *
 *	History:
 *		- 18/05/2006: Added support for panning
 *		- 28/05/2006: Bug corrected in RemoveAllSeries
 *		- 28/05/2006: Added support for resizing
 *		- 12/06/2006: Added support for manual zoom
 *		- 10/08/2006: Added SetZoomMinMax and UndoZoom
 *		- 24/03/2007: GDI leak corrected
 *		- 24/03/2007: Invisible series are not taken in account for auto axis 
 *					  and legend (thanks to jerminator-jp).
 *		- 24/03/2007: possibility to specify a margin for the axis. Needs to be improved
 *		- 05/04/2007: ability to change the text color of the axis.
 *		- 05/04/2007: ability to change the color of the border of the drawing area.
 *		- 05/04/2007: Surface series added.
 *		- 26/08/2007: The clipping area of the series is a bit larger (they will be
 *					  drawn over the bottom and right axes).
 *		- 12/01/2007: Ability to change the color of the zoom rectangle.
 *		- 08/02/2008: Added convenience functions to convert from date to value and 
 *					  opposite.
 *		- 21/02/2008: The zoom doesn't do anything if the user only clicks on the control
 *					  (thanks to Eugene Pustovoyt).
 *		- 29/02/2008: The auto axis are now refreshed when a series is removed (thanks to
 *					  Bruno Lavier).
 *		- 08/03/2008: EnableRefresh function added (thanks to Bruno Lavier).
 *		- 21/03/2008: Added support for scrolling.
 *		- 25/03/2008: UndoZoom function added.
 *		- 25/03/2008: A series can now be removed using its pointer.
 *		- 12/08/2008: Performance patch (thanks to Nick Holgate).
 *		- 18/08/2008: Added support for printing.
 *		- 31/10/2008: Fixed a bug for unicode.
 *
 */

#include "stdafx.h"
#include "ChartCtrl.h"

#include "ChartSerie.h"
#include "ChartGradient.h"
#include "ChartStandardAxis.h"
#include "ChartDateTimeAxis.h"
#include "ChartLogarithmicAxis.h"
#include "ChartCrossHairCursor.h"
#include "ChartDragLineCursor.h"

#include "ChartPointsSerie.h"
#include "ChartLineSerie.h"
#include "ChartSurfaceSerie.h"
#include "ChartBarSerie.h"
#include "ChartCandlestickSerie.h"
#include "ChartGanttSerie.h"

#if _MFC_VER > 0x0600
#include "atlImage.h"
#endif

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CHARTCTRL_CLASSNAME    _T("ChartCtrl")  // Window class name


COLORREF pSeriesColorTable[] = { RGB(255,0,0), RGB(0,150,0), RGB(0,0,255), RGB(255,255,0), RGB(0,255,255), 
								 RGB(255,128,0), RGB(128,0,128), RGB(128,128,0), RGB(255,0,255), RGB(64,128,128)};

/////////////////////////////////////////////////////////////////////////////
// CChartCtrl

CChartCtrl::CChartCtrl()
{
	RegisterWindowClass();

	m_iEnableRefresh = 1;
	m_bPendingRefresh = false;
	m_BorderColor = RGB(0,0,0);
	m_BackColor = GetSysColor(COLOR_BTNFACE);
	EdgeType = EDGE_RAISED;
	m_BackGradientType = gtVertical;
	m_bBackGradient = false;
	m_BackGradientCol1 = m_BackGradientCol2 = m_BackColor;

	for (int i=0;i<4;i++)
		m_pAxes[i] = NULL;

	m_pLegend = new CChartLegend(this);
	m_pTitles = new CChartTitle(this);
	
	m_bMemDCCreated = false;
	m_bPanEnabled = true;
	m_bRMouseDown = false;

	m_bZoomEnabled = true;
	m_bLMouseDown = false;
	m_ZoomRectColor = RGB(255,255,255);

	m_bToolBarCreated = false;
	m_pMouseListener = NULL;
	m_bMouseVisible = true;
}

CChartCtrl::~CChartCtrl()
{
	TSeriesMap::iterator seriesIter = m_mapSeries.begin();
	for (seriesIter; seriesIter!=m_mapSeries.end(); seriesIter++)
	{
		delete (seriesIter->second);
	}
	m_mapSeries.clear();
	TCursorMap::iterator cursorIter = m_mapCursors.begin();
	for (cursorIter; cursorIter!=m_mapCursors.end(); cursorIter++)
	{
		delete (cursorIter->second);
	}

	for (int i=0; i<4 ;i++)
	{
		if (m_pAxes[i])
			delete m_pAxes[i];
	}

	if (m_pLegend)
	{
		delete m_pLegend;
		m_pLegend = NULL;
	}
	if (m_pTitles)
	{
		delete m_pTitles;
		m_pTitles = NULL;
	}
}


BEGIN_MESSAGE_MAP(CChartCtrl, CWnd)
	//{{AFX_MSG_MAP(CChartCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CChartCtrl message handlers

void CChartCtrl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting


	//创建背景DC，绘制背景
	if (!m_BackgroundDC.GetSafeHdc())
	{
		CRect ClientRect;
		GetClientRect(&ClientRect);
		CBitmap memBitmap;
		m_BackgroundDC.CreateCompatibleDC(&dc);
		memBitmap.CreateCompatibleBitmap(&dc, ClientRect.Width(), ClientRect.Height());
		m_BackgroundDC.SelectObject(&memBitmap);
		m_bMemDCCreated = true;
		RefreshChart();
	}



    // Get Size of Display area
    CRect rect;
    GetClientRect(&rect);
	dc.BitBlt(0, 0, rect.Width(), rect.Height(), 
			  &m_BackgroundDC, 0, 0, SRCCOPY) ;

	//绘制缩放范围矩形框
	// Draw the zoom rectangle
	if (m_bZoomEnabled && m_bLMouseDown)
	{
		CPen NewPen(PS_SOLID,1,m_ZoomRectColor);
		CPen* pOldPen = dc.SelectObject(&NewPen);

		dc.MoveTo(m_rectZoomArea.TopLeft());
		dc.LineTo(m_rectZoomArea.right,m_rectZoomArea.top);
		dc.LineTo(m_rectZoomArea.BottomRight());
		dc.LineTo(m_rectZoomArea.left,m_rectZoomArea.bottom);
		dc.LineTo(m_rectZoomArea.TopLeft());

		dc.SelectObject(pOldPen);
		DeleteObject(NewPen);
	}

	// 绘制鼠标
	TCursorMap::iterator iter = m_mapCursors.begin();
	for (iter; iter!=m_mapCursors.end(); iter++)
		iter->second->Draw(&dc);
}

BOOL CChartCtrl::OnEraseBkgnd(CDC* pDC)
{
	// To avoid flickering
//	return CWnd::OnEraseBkgnd(pDC);
	return FALSE;
}

CChartCrossHairCursor* CChartCtrl::CreateCrossHairCursor(bool bSecondaryHorizAxis, 
														 bool bSecondaryVertAxis)
{
	CChartAxis* pHorizAxis = NULL;
	CChartAxis* pVertAxis = NULL;
	if (bSecondaryHorizAxis)
		pHorizAxis = m_pAxes[TopAxis];
	else
		pHorizAxis = m_pAxes[BottomAxis];
	if (bSecondaryVertAxis)
		pVertAxis = m_pAxes[RightAxis];
	else
		pVertAxis = m_pAxes[LeftAxis];

	ASSERT(pHorizAxis != NULL);
	ASSERT(pVertAxis != NULL);

	CChartCrossHairCursor* pNewCursor = new CChartCrossHairCursor(this, pHorizAxis, pVertAxis);
	m_mapCursors[pNewCursor->GetCursorId()] = pNewCursor;
	return pNewCursor;
}

CChartDragLineCursor* CChartCtrl::CreateDragLineCursor(EAxisPos relatedAxis)
{
	ASSERT(m_pAxes[relatedAxis] != NULL);

	CChartDragLineCursor* pNewCursor = new CChartDragLineCursor(this, m_pAxes[relatedAxis]);
	m_mapCursors[pNewCursor->GetCursorId()] = pNewCursor;
	return pNewCursor;
}

void CChartCtrl::AttachCustomCursor(CChartCursor* pCursor)
{
	m_mapCursors[pCursor->GetCursorId()] = pCursor;
}

void CChartCtrl::RemoveCursor(unsigned cursorId)
{
	TCursorMap::iterator iter = m_mapCursors.find(cursorId);
	if (iter != m_mapCursors.end())
	{
		delete iter->second;
		m_mapCursors.erase(iter);
	}
}

void CChartCtrl::ShowMouseCursor(bool bShow)
{
	m_bMouseVisible = bShow;
	if (!bShow)
		SetCursor(NULL);
}

CChartStandardAxis* CChartCtrl::CreateStandardAxis(EAxisPos axisPos)
{
	CChartStandardAxis* pAxis = new CChartStandardAxis();
	AttachCustomAxis(pAxis, axisPos);
	return pAxis;
}

CChartLogarithmicAxis* CChartCtrl::CreateLogarithmicAxis(EAxisPos axisPos)
{
	CChartLogarithmicAxis* pAxis = new CChartLogarithmicAxis();
	AttachCustomAxis(pAxis, axisPos);
	return pAxis;
}

CChartDateTimeAxis* CChartCtrl::CreateDateTimeAxis(EAxisPos axisPos)
{
	CChartDateTimeAxis* pAxis = new CChartDateTimeAxis();
	AttachCustomAxis(pAxis, axisPos);
	return pAxis;
}

void CChartCtrl::AttachCustomAxis(CChartAxis* pAxis, EAxisPos axisPos)
{
	// The axis should not be already attached to another control
	ASSERT(pAxis->m_pParentCtrl == NULL);
	pAxis->SetParent(this);

	//设置辅助轴和水平轴标记
	if ( (axisPos==RightAxis) || (axisPos==TopAxis) )
		pAxis->SetSecondary(true);
	if (  (axisPos==BottomAxis) || (axisPos==TopAxis) )
		pAxis->SetHorizontal(true);
	else
		pAxis->SetHorizontal(false);
	//创建滚动条
	pAxis->CreateScrollBar();

	// Beofre storing the new axis, we should delete the previous one if any删除原坐标轴对象
	if (m_pAxes[axisPos])
		delete m_pAxes[axisPos];
	m_pAxes[axisPos] = pAxis;
}

bool CChartCtrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if (!(::GetClassInfo(hInst, CHARTCTRL_CLASSNAME, &wndcls)))
    {
		memset(&wndcls, 0, sizeof(WNDCLASS));   

		wndcls.hInstance		= hInst;
		wndcls.lpfnWndProc		= ::DefWindowProc;
		wndcls.hCursor			= NULL; //LoadCursor(NULL, IDC_ARROW);
		wndcls.hIcon			= 0;
		wndcls.lpszMenuName		= NULL;
		wndcls.hbrBackground	= (HBRUSH) ::GetStockObject(WHITE_BRUSH);
		wndcls.style			= CS_DBLCLKS; 
		wndcls.cbClsExtra		= 0;
		wndcls.cbWndExtra		= 0;
		wndcls.lpszClassName    = CHARTCTRL_CLASSNAME;

        if (!RegisterClass(&wndcls))
        {
          //  AfxThrowResourceException();
            return false;
        }
    }

    return true;

}

int CChartCtrl::Create(CWnd *pParentWnd, const RECT &rect, UINT nID, DWORD dwStyle)
{
	dwStyle |= WS_CLIPCHILDREN;
	int Result = CWnd::Create(CHARTCTRL_CLASSNAME, _T(""), dwStyle, rect, pParentWnd, nID);

	//if (Result)
	//	RefreshChart();

	return Result;
}

void CChartCtrl::SetBackGradient(COLORREF Col1, COLORREF Col2, EGradientType GradientType)
{
	m_bBackGradient = true;
	m_BackGradientCol1 = Col1;
	m_BackGradientCol2 = Col2;
	m_BackGradientType = GradientType;
	//RefreshChart();
}

//void CChartCtrl::SetChartCtrlRect(CRect rect)
//{
//	m_ChartRect = rect;
//	AdaptPlottingRect();
//}


void CChartCtrl::SetPlottingRect(CRect rect)
{
	m_PlottingRect = rect;

}


void CChartCtrl::EnableRefresh(bool bEnable)
{
	if (bEnable)
		m_iEnableRefresh++;
	else
		m_iEnableRefresh--;
	if (m_iEnableRefresh > 0 && m_bPendingRefresh)
	{
		m_bPendingRefresh = false;
		RefreshChart();
	}
}

void CChartCtrl::UndoPanZoom()
{
	//EnableRefresh(false);
	if (m_pAxes[BottomAxis])
		m_pAxes[BottomAxis]->UndoZoom();
	if (m_pAxes[LeftAxis])
		m_pAxes[LeftAxis]->UndoZoom();
	if (m_pAxes[TopAxis])
		m_pAxes[TopAxis]->UndoZoom();
	if (m_pAxes[RightAxis])
		m_pAxes[RightAxis]->UndoZoom();
	//EnableRefresh(true);
}

void CChartCtrl::RefreshChart()
{
	// Window is not created yet, so skip the refresh.
	if (!GetSafeHwnd())
		return;

	//判断是否刷新
	//if (m_iEnableRefresh < 1)
	//{
	//	m_bPendingRefresh = true;
	//	return;
	//}

	// Retrieve the client rect and initialize the plotting rect
	CClientDC dc(this) ;  
	CRect ClientRect;
	GetClientRect(&ClientRect);
	
	
	// If the backgroundDC was not created yet, create it (it is used to avoid flickering).
	//创建背景DC，绘制背景
	//if (!m_BackgroundDC.GetSafeHdc() )
	//{
	//	CBitmap memBitmap;
	//	m_BackgroundDC.CreateCompatibleDC(&dc) ;
	//	memBitmap.CreateCompatibleBitmap(&dc, ClientRect.Width(),ClientRect.Height()) ;
	//	m_BackgroundDC.SelectObject(&memBitmap) ;
	//}

	if (m_BackgroundDC.GetSafeHdc())
	{
		//绘制背景
		DrawBackground(&m_BackgroundDC, ClientRect);
		
		//绘制表格
		ClientRect.DeflateRect(40, 40);
 
		DrawChart(&m_BackgroundDC, ClientRect);
	}


	

	//刷新窗口
	Invalidate();
}

void CChartCtrl::GetFigureRect(CDC* pDC, CRect PlottingRect, CRect& FigureRect)
{
    FigureRect = PlottingRect;
	CRect PreFigureRect;

	int  n = 0;
	do
	{
		PreFigureRect = FigureRect;
		for (n = 0; n < 4; n++)
		{
			if (m_pAxes[n])
			{
				m_pAxes[n]->SetAxisSize(PlottingRect, FigureRect);
				m_pAxes[n]->RefreshTicks(pDC);
			}
		}

		AdjustPlotingRect(pDC, PlottingRect, FigureRect);
 
	} while (PreFigureRect != FigureRect);

}


void CChartCtrl::AdjustPlotingRect(CDC* pDC, CRect PlottingRect, CRect& FigureRect)
{
	int HorMarginSize;
	int VerMarginSize;
	int n = 0;
	for (n = 0; n < 4; n++)
	{
		if (m_pAxes[n])
		{
			m_pAxes[n]->RefreshMarginSize(pDC);
			m_pAxes[n]->GetMarginSize(HorMarginSize, VerMarginSize);
			if (m_pAxes[n]->IsHorizontal())
			{
				//水平轴

				if (!m_pAxes[n]->IsSecondary())
				{

					if (PlottingRect.bottom - VerMarginSize < FigureRect.bottom)
						FigureRect.bottom = PlottingRect.bottom - VerMarginSize;

					if (PlottingRect.right - HorMarginSize < FigureRect.right)
						FigureRect.right = PlottingRect.right - VerMarginSize;
				}
				else
				{
					//top轴


					if (PlottingRect.top + VerMarginSize  > FigureRect.top)
						FigureRect.top = PlottingRect.top + VerMarginSize;

					if (PlottingRect.right - HorMarginSize < FigureRect.right)
						FigureRect.right = PlottingRect.right - HorMarginSize;
				}

			}
			else
			{

				if (!m_pAxes[n]->IsSecondary())
				{


					if (PlottingRect.top + VerMarginSize > FigureRect.top)
						FigureRect.top = PlottingRect.top + VerMarginSize;

					if (PlottingRect.left + HorMarginSize > FigureRect.left)
						FigureRect.left = PlottingRect.left + HorMarginSize;
				}
				else
				{


					if (PlottingRect.top + VerMarginSize > FigureRect.top)
						FigureRect.top = PlottingRect.top + VerMarginSize;

					if (PlottingRect.right - HorMarginSize < FigureRect.right)
						FigureRect.right = PlottingRect.right - HorMarginSize;
				}
			}

		}
	}
 
}


void CChartCtrl::DrawFigure(CDC* pDC, CRect FigureRect)
{
 
	
	//CRect  FigureRect = m_PlottingRect;


	//确定图形和坐标轴区域
 
	CRect ClientRect;
	GetClientRect(&ClientRect);
	int n = 0;
 
	for (n = 0; n<4; n++)
	{
		if (m_pAxes[n])
		{
			m_pAxes[n]->SetAxisSize(ClientRect, FigureRect);
			//m_pAxes[n]->RefreshTicks(pDC);
			m_pAxes[n]->Draw(pDC);
		}
	}

	//移动滚动条位置
	for (int i = 0; i<4; i++)
	{
		if (m_pAxes[i])
			m_pAxes[i]->UpdateScrollBarPos();
	}


	//绘制绘图区域边框
	CPen SolidPen(PS_SOLID, 0, m_BorderColor);
	CPen* pOldPen = pDC->SelectObject(&SolidPen);

	pDC->MoveTo(FigureRect.left, FigureRect.top);
	pDC->LineTo(FigureRect.right, FigureRect.top);
	pDC->LineTo(FigureRect.right, FigureRect.bottom);
	pDC->LineTo(FigureRect.left, FigureRect.bottom);
	pDC->LineTo(FigureRect.left, FigureRect.top);

	pDC->SelectObject(pOldPen);
	DeleteObject(SolidPen);

	//绘制图形
	TSeriesMap::iterator iter = m_mapSeries.begin();
	for (iter; iter != m_mapSeries.end(); iter++)
	{
		CRect drawingRect = FigureRect;
		drawingRect.top += 5;
		drawingRect.bottom -= 4;
		drawingRect.right += 1;
		iter->second->SetPlottingRect(drawingRect);
		iter->second->DrawAll(pDC);
	}

	// 绘制标记
	pDC->IntersectClipRect(FigureRect);
	for (iter = m_mapSeries.begin(); iter != m_mapSeries.end(); iter++)
	{
		iter->second->DrawLabels(pDC);
	}

	pDC->SelectClipRgn(NULL);


}


void CChartCtrl::DrawChart(CDC* pDC, CRect ChartRect)
{
	//设置绘图区
	//m_PlottingRect = ChartRect;
	


	CRect rcTitle;
	rcTitle = ChartRect;
	CSize TitleSize = m_pTitles->GetSize(pDC);
	rcTitle.bottom = TitleSize.cy;
	m_pTitles->SetTitleRect(rcTitle);


	m_pTitles->Draw(pDC);

 
	//ChartRect.bottom +=2500;
	ChartRect.top += TitleSize.cy;
	m_pLegend->ClipArea(ChartRect, pDC);

	//SetPlottingRect(ChartRect);
	//Clip all the margins (axis) of the client rect
	CRect  FigureRect;
	GetFigureRect(pDC, ChartRect, m_PlottingRect);
	DrawFigure(pDC, m_PlottingRect);



	// 绘制图例.
	m_pLegend->Draw(pDC);
}

void CChartCtrl::DrawBackground(CDC* pDC, CRect ChartRect)
{
	CBrush BrushBack;
	BrushBack.CreateSolidBrush(m_BackColor) ;
	if (!m_bBackGradient)
	{
		pDC->SetBkColor(m_BackColor);
		pDC->FillRect(ChartRect,&BrushBack);
	}
	else
	{
		CChartGradient::DrawGradient(pDC,ChartRect,m_BackGradientCol1,
									 m_BackGradientCol2,m_BackGradientType);
	}

	// Draw the edge.
	//pDC->DrawEdge(ChartRect,EdgeType,BF_RECT);
}

void CChartCtrl::RefreshScreenAutoAxes()
{
	for (int n=0;n<4;n++)
	{
		if (m_pAxes[n])
			m_pAxes[n]->RefreshScreenAutoAxis();
	}
}

CChartPointsSerie* CChartCtrl::CreatePointsSerie(bool bSecondaryHorizAxis, 
											   bool bSecondaryVertAxis)
{
	CChartPointsSerie* pNewSerie = new CChartPointsSerie(this);
	AttachCustomSerie(pNewSerie, bSecondaryHorizAxis, bSecondaryVertAxis);
	return pNewSerie;
}

CChartLineSerie* CChartCtrl::CreateLineSerie(bool bSecondaryHorizAxis, 
											 bool bSecondaryVertAxis)
{
	CChartLineSerie* pNewSerie = new CChartLineSerie(this);
	AttachCustomSerie(pNewSerie, bSecondaryHorizAxis, bSecondaryVertAxis);
	return pNewSerie;
}

CChartSurfaceSerie* CChartCtrl::CreateSurfaceSerie(bool bSecondaryHorizAxis, 
												   bool bSecondaryVertAxis)
{
	CChartSurfaceSerie* pNewSerie = new CChartSurfaceSerie(this);
	AttachCustomSerie(pNewSerie, bSecondaryHorizAxis, bSecondaryVertAxis);
	return pNewSerie;
}

CChartBarSerie* CChartCtrl::CreateBarSerie(bool bSecondaryHorizAxis, 
										   bool bSecondaryVertAxis)
{
	CChartBarSerie* pNewSerie = new CChartBarSerie(this);
	AttachCustomSerie(pNewSerie, bSecondaryHorizAxis, bSecondaryVertAxis);
	return pNewSerie;
}

CChartCandlestickSerie* CChartCtrl::CreateCandlestickSerie(bool bSecondaryHorizAxis, 
														   bool bSecondaryVertAxis)
{
	CChartCandlestickSerie* pNewSerie = new CChartCandlestickSerie(this);
	AttachCustomSerie(pNewSerie, bSecondaryHorizAxis, bSecondaryVertAxis);
	return pNewSerie;
}

CChartGanttSerie* CChartCtrl::CreateGanttSerie(bool bSecondaryHorizAxis, 
													 bool bSecondaryVertAxis)
{
	CChartGanttSerie* pNewSerie = new CChartGanttSerie(this);
	AttachCustomSerie(pNewSerie, bSecondaryHorizAxis, bSecondaryVertAxis);
	return pNewSerie;
}

void CChartCtrl::AttachCustomSerie(CChartSerie* pNewSeries,
								   bool bSecondaryHorizAxis,
								   bool bSecondaryVertAxis)
{
	size_t ColIndex = m_mapSeries.size()%10;

	CChartAxis* pHorizAxis = NULL;
	CChartAxis* pVertAxis = NULL;
	if (bSecondaryHorizAxis)
		pHorizAxis = m_pAxes[TopAxis];
	else
		pHorizAxis = m_pAxes[BottomAxis];
	if (bSecondaryVertAxis)
		pVertAxis = m_pAxes[RightAxis];
	else
		pVertAxis = m_pAxes[LeftAxis];

	ASSERT(pHorizAxis != NULL);
	ASSERT(pVertAxis != NULL);

	if (pNewSeries)
	{
		pNewSeries->SetPlottingRect(m_PlottingRect);
		pNewSeries->SetColor(pSeriesColorTable[ColIndex]);
		pNewSeries->m_pHorizontalAxis = pHorizAxis;
		pNewSeries->m_pVerticalAxis = pVertAxis;
		m_mapSeries[pNewSeries->GetSerieId()] = pNewSeries;

		EnableRefresh(false);
		pVertAxis->RegisterSeries(pNewSeries);
		pVertAxis->AutoRefreshAxis();
		pHorizAxis->RegisterSeries(pNewSeries);
		pHorizAxis->AutoRefreshAxis();

		// The series will need to be redrawn so we need to refresh the control
		RefreshChart();
		EnableRefresh(true);
	}
}

CChartSerie* CChartCtrl::GetSerie(size_t uSerieId) const
{
	CChartSerie* pToReturn = NULL;
	TSeriesMap::const_iterator iter = m_mapSeries.find(uSerieId);
	if (iter != m_mapSeries.end())
	{
		pToReturn = iter->second;
	}

	return pToReturn;
}

void CChartCtrl::RemoveSerie(unsigned uSerieId)
{
	TSeriesMap::iterator iter = m_mapSeries.find(uSerieId);
	if (iter != m_mapSeries.end())
	{
		CChartSerie* pToDelete = iter->second;
		m_mapSeries.erase(iter);

		//EnableRefresh(false);
		pToDelete->m_pVerticalAxis->UnregisterSeries(pToDelete);
		pToDelete->m_pHorizontalAxis->UnregisterSeries(pToDelete);
		pToDelete->m_pVerticalAxis->AutoRefreshAxis();
		pToDelete->m_pHorizontalAxis->AutoRefreshAxis();
		delete pToDelete;
		RefreshChart();
		//EnableRefresh(true);
	}
}

void CChartCtrl::RemoveAllSeries()
{
	TSeriesMap::iterator iter = m_mapSeries.begin();
	for (iter; iter != m_mapSeries.end(); iter++)
	{
		delete iter->second;
	}

	m_mapSeries.clear();
 
}


CChartAxis* CChartCtrl::GetBottomAxis() const
{
	return (m_pAxes[BottomAxis]);
}

CChartAxis* CChartCtrl::GetLeftAxis() const
{
	return (m_pAxes[LeftAxis]);
}

CChartAxis* CChartCtrl::GetTopAxis() const
{
	return (m_pAxes[TopAxis]);
}

CChartAxis* CChartCtrl::GetRightAxis() const
{
	return (m_pAxes[RightAxis]);
}


CDC* CChartCtrl::GetDC()
{
	return &m_BackgroundDC;
}


size_t CChartCtrl::GetSeriesCount() const
{
	return m_mapSeries.size();
}

/////////////////////////////////////////////////////////////////////////////
// Mouse events

void CChartCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	//右键窗口漫游
	if (m_bRMouseDown && m_bPanEnabled)
	{
		if (point != m_PanAnchor)
		{
			//EnableRefresh(false);
			if (m_pAxes[LeftAxis])
				m_pAxes[LeftAxis]->PanAxis(m_PanAnchor.y,point.y);
			if (m_pAxes[RightAxis])
				m_pAxes[RightAxis]->PanAxis(m_PanAnchor.y,point.y);
			if (m_pAxes[BottomAxis])
				m_pAxes[BottomAxis]->PanAxis(m_PanAnchor.x,point.x);
			if (m_pAxes[TopAxis])
				m_pAxes[TopAxis]->PanAxis(m_PanAnchor.x,point.x);
			RefreshChart();
			//EnableRefresh(true);
			// Force an immediate repaint of the window, so that the mouse messages
			// are by passed (this allows for a smooth pan)
			UpdateWindow();

			m_PanAnchor = point;
		}
	}

	//左键窗口缩放
	if (m_bLMouseDown && m_bZoomEnabled)
	{
		m_rectZoomArea.BottomRight() = point;
		Invalidate();
	}
	//坐标轴滚动条响应
	//隐藏滚动条
	for (int i=0; i<4; i++)
	{
		if (m_pAxes[i])
			m_pAxes[i]->m_pScrollBar->OnMouseLeave();
	}
	//获取鼠标位置子窗口
	CWnd* pWnd = ChildWindowFromPoint(point);
	//鼠标位于滚动条，显示滚动条
	if (pWnd != this)
	{
		CChartScrollBar* pScrollBar = dynamic_cast<CChartScrollBar*>(pWnd);
		if (pScrollBar)
			pScrollBar->OnMouseEnter();
	}
	 
	if (m_PlottingRect.PtInRect(point))
	{
		//鼠标位于绘图区，响应鼠标位置信息
		TCursorMap::iterator iter = m_mapCursors.begin();
		for (iter; iter!=m_mapCursors.end(); iter++)
			iter->second->OnMouseMove(point);
		
		Invalidate();
	}

	//设置鼠标光标
	if (!m_bMouseVisible && m_PlottingRect.PtInRect(point))
		SetCursor(NULL);
	else
		SetCursor(::LoadCursor(NULL,IDC_ARROW));

	//发送鼠标移动消息
	SendMouseEvent(CChartMouseListener::MouseMove, point);

	CWnd::OnMouseMove(nFlags, point);
}

void CChartCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	SetCapture();
	if (m_bZoomEnabled)
	{
		m_bLMouseDown = true;
		m_rectZoomArea.TopLeft() = point;
		m_rectZoomArea.BottomRight() = point;
	}

	if (m_PlottingRect.PtInRect(point))
	{
		TCursorMap::iterator iter = m_mapCursors.begin();
		for (iter; iter!=m_mapCursors.end(); iter++)
			iter->second->OnMouseButtonDown(point);
		
		Invalidate();
	}

	SendMouseEvent(CChartMouseListener::LButtonDown, point);
	CWnd::OnLButtonDown(nFlags, point);
}

void CChartCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	m_bLMouseDown = false;
	if (m_bZoomEnabled)
	{
		if ( (m_rectZoomArea.left > m_rectZoomArea.right) ||
			 (m_rectZoomArea.top > m_rectZoomArea.bottom))
		{
			UndoPanZoom();
		}
		else if ( (m_rectZoomArea.left!=m_rectZoomArea.right) &&
				  (m_rectZoomArea.top!=m_rectZoomArea.bottom))
		{
			double MinVal = 0;			
			double MaxVal = 0;
			
			if (m_pAxes[BottomAxis])
			{
				if (m_pAxes[BottomAxis]->IsInverted())
				{
					MaxVal = m_pAxes[BottomAxis]->ScreenToValue(m_rectZoomArea.left);
					MinVal = m_pAxes[BottomAxis]->ScreenToValue(m_rectZoomArea.right);
				}
				else
				{
					MinVal = m_pAxes[BottomAxis]->ScreenToValue(m_rectZoomArea.left);
					MaxVal = m_pAxes[BottomAxis]->ScreenToValue(m_rectZoomArea.right);
				}
				m_pAxes[BottomAxis]->SetZoomMinMax(MinVal,MaxVal);
			}

			if (m_pAxes[LeftAxis])
			{
				if (m_pAxes[LeftAxis]->IsInverted())
				{
					MaxVal = m_pAxes[LeftAxis]->ScreenToValue(m_rectZoomArea.bottom);
					MinVal = m_pAxes[LeftAxis]->ScreenToValue(m_rectZoomArea.top);
				}
				else
				{
					MinVal = m_pAxes[LeftAxis]->ScreenToValue(m_rectZoomArea.bottom);
					MaxVal = m_pAxes[LeftAxis]->ScreenToValue(m_rectZoomArea.top);
				}
				m_pAxes[LeftAxis]->SetZoomMinMax(MinVal,MaxVal);
			}

			if (m_pAxes[TopAxis])
			{
				if (m_pAxes[TopAxis]->IsInverted())
				{
					MaxVal = m_pAxes[TopAxis]->ScreenToValue(m_rectZoomArea.left);
					MinVal = m_pAxes[TopAxis]->ScreenToValue(m_rectZoomArea.right);
				}
				else
				{
					MinVal = m_pAxes[TopAxis]->ScreenToValue(m_rectZoomArea.left);
					MaxVal = m_pAxes[TopAxis]->ScreenToValue(m_rectZoomArea.right);
				}
				m_pAxes[TopAxis]->SetZoomMinMax(MinVal,MaxVal);
			}

			if (m_pAxes[RightAxis])
			{
				if (m_pAxes[RightAxis]->IsInverted())
				{
					MaxVal = m_pAxes[RightAxis]->ScreenToValue(m_rectZoomArea.bottom);
					MinVal = m_pAxes[RightAxis]->ScreenToValue(m_rectZoomArea.top);
				}
				else
				{
					MinVal = m_pAxes[RightAxis]->ScreenToValue(m_rectZoomArea.bottom);
					MaxVal = m_pAxes[RightAxis]->ScreenToValue(m_rectZoomArea.top);
				}
				m_pAxes[RightAxis]->SetZoomMinMax(MinVal,MaxVal);
			}

			RefreshChart();
		}
	}

	if (m_PlottingRect.PtInRect(point))
	{
		TCursorMap::iterator iter = m_mapCursors.begin();
		for (iter; iter!=m_mapCursors.end(); iter++)
			iter->second->OnMouseButtonUp(point);
		
		Invalidate();
	}

	SendMouseEvent(CChartMouseListener::LButtonUp, point);
	CWnd::OnLButtonUp(nFlags, point);
}

void CChartCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	SendMouseEvent(CChartMouseListener::LButtonDoubleClick, point);
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CChartCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	SetCapture();
	m_bRMouseDown = true;
	if (m_bPanEnabled)
		m_PanAnchor = point;

	SendMouseEvent(CChartMouseListener::RButtonDown, point);
	CWnd::OnRButtonDown(nFlags, point);
}

void CChartCtrl::OnRButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	m_bRMouseDown = false;

	SendMouseEvent(CChartMouseListener::RButtonUp, point);
	CWnd::OnRButtonUp(nFlags, point);
}

void CChartCtrl::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	SendMouseEvent(CChartMouseListener::RButtonDoubleClick, point);
	CWnd::OnRButtonDblClk(nFlags, point);
}

void CChartCtrl::SendMouseEvent(CChartMouseListener::MouseEvent mouseEvent, 
								const CPoint& screenPoint) const
{
	if (m_pMouseListener)
	{
		// Check where the click occured.
		if (m_pTitles->IsPointInside(screenPoint))
			m_pMouseListener->OnMouseEventTitle(mouseEvent,screenPoint);
		if (m_pLegend->IsPointInside(screenPoint))
			m_pMouseListener->OnMouseEventLegend(mouseEvent,screenPoint);
		for (int i=0; i<4; i++)
		{
			if ( m_pAxes[i] && m_pAxes[i]->IsPointInside(screenPoint) )
				m_pMouseListener->OnMouseEventAxis(mouseEvent,screenPoint,m_pAxes[i]);
		}
		if (m_PlottingRect.PtInRect(screenPoint))
			m_pMouseListener->OnMouseEventPlotArea(mouseEvent,screenPoint);
	}
	
	// Check all the series in reverse order (check the series on top first).
	TSeriesMap::const_reverse_iterator rIter = m_mapSeries.rbegin();
	for(rIter; rIter!=m_mapSeries.rend(); rIter++)
	{
		if (rIter->second->OnMouseEvent(mouseEvent, screenPoint))
			break;
	}
}

void CChartCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	// Force recreation of background DC
	if (m_BackgroundDC.GetSafeHdc() )
		m_BackgroundDC.DeleteDC();
	
	RefreshChart();
}

double CChartCtrl::DateToValue(const COleDateTime& Date)
{
	return (DATE)Date;
}

COleDateTime CChartCtrl::ValueToDate(double Value)
{
	COleDateTime RetDate((DATE)Value);
	return RetDate;
}

void CChartCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CChartScrollBar* pChartBar = dynamic_cast<CChartScrollBar*>(pScrollBar);
	if (pChartBar)
		pChartBar->OnHScroll(nSBCode, nPos);

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
	RefreshChart();
}

void CChartCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CChartScrollBar* pChartBar = dynamic_cast<CChartScrollBar*>(pScrollBar);
	if (pChartBar)
		pChartBar->OnVScroll(nSBCode, nPos);

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
	RefreshChart();
}

void CChartCtrl::Print(const TChartString& strTitle, CPrintDialog* pPrntDialog)
{
	CDC dc;
    if (pPrntDialog == NULL)
    {
        CPrintDialog printDlg(FALSE);
        if (printDlg.DoModal() != IDOK)         // Get printer settings from user
            return;

		dc.Attach(printDlg.GetPrinterDC());     // attach a printer DC
    }
    else
		dc.Attach(pPrntDialog->GetPrinterDC()); // attach a printer DC
    dc.m_bPrinting = TRUE;
	
    DOCINFO di;                                 // Initialise print doc details
    memset(&di, 0, sizeof (DOCINFO));
    di.cbSize = sizeof (DOCINFO);
	di.lpszDocName = strTitle.c_str();

    BOOL bPrintingOK = dc.StartDoc(&di);        // Begin a new print job

    CPrintInfo Info;
    Info.m_rectDraw.SetRect(0,0, dc.GetDeviceCaps(HORZRES), dc.GetDeviceCaps(VERTRES));

    OnBeginPrinting(&dc, &Info);                // Initialise printing
    for (UINT page = Info.GetMinPage(); page <= Info.GetMaxPage() && bPrintingOK; page++)
    {
        dc.StartPage();                         // begin new page
        Info.m_nCurPage = page;
        OnPrint(&dc, &Info);                    // Print page
        bPrintingOK = (dc.EndPage() > 0);       // end page
    }
    OnEndPrinting(&dc, &Info);                  // Clean up after printing

    if (bPrintingOK)
        dc.EndDoc();                            // end a print job
    else
        dc.AbortDoc();                          // abort job.

    dc.Detach();                                // detach the printer DC
}

void CChartCtrl::OnBeginPrinting(CDC *pDC, CPrintInfo *pInfo)
{
    // OnBeginPrinting() is called after the user has committed to
    // printing by OK'ing the Print dialog, and after the framework
    // has created a CDC object for the printer or the preview view.

    // This is the right opportunity to set up the page range.
    // Given the CDC object, we can determine how many rows will
    // fit on a page, so we can in turn determine how many printed
    // pages represent the entire document.
    ASSERT(pDC && pInfo);

    // Get a DC for the current window (will be a screen DC for print previewing)
    CDC *pCurrentDC = GetDC();        // will have dimensions of the client area
    if (!pCurrentDC) 
		return;

    CSize PaperPixelsPerInch(pDC->GetDeviceCaps(LOGPIXELSX), pDC->GetDeviceCaps(LOGPIXELSY));
    CSize ScreenPixelsPerInch(pCurrentDC->GetDeviceCaps(LOGPIXELSX), pCurrentDC->GetDeviceCaps(LOGPIXELSY));

    // Create the printer font
    int nFontSize = -10;
    CString strFontName = _T("Arial");
    m_PrinterFont.CreateFont(nFontSize, 0,0,0, FW_NORMAL, 0,0,0, DEFAULT_CHARSET,
                             OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, strFontName);
    CFont *pOldFont = pDC->SelectObject(&m_PrinterFont);

    // Get the page sizes (physical and logical)
    m_PaperSize = CSize(pDC->GetDeviceCaps(HORZRES), pDC->GetDeviceCaps(VERTRES));

	m_LogicalPageSize.cx = ScreenPixelsPerInch.cx * m_PaperSize.cx / PaperPixelsPerInch.cx * 3 / 4;
	m_LogicalPageSize.cy = ScreenPixelsPerInch.cy * m_PaperSize.cy / PaperPixelsPerInch.cy * 3 / 4;


    // Set up the print info
    pInfo->SetMaxPage(1);
    pInfo->m_nCurPage = 1;                        // start printing at page# 1

    ReleaseDC(pCurrentDC);
    pDC->SelectObject(pOldFont);
}

void CChartCtrl::OnPrint(CDC *pDC, CPrintInfo *pInfo)
{
    if (!pDC || !pInfo)
        return;

    CFont *pOldFont = pDC->SelectObject(&m_PrinterFont);

    // Set the page map mode to use GraphCtrl units
	pDC->SetMapMode(MM_ANISOTROPIC);
    pDC->SetWindowExt(m_LogicalPageSize);
    pDC->SetViewportExt(m_PaperSize);
    pDC->SetWindowOrg(0, 0);

    // Header
    pInfo->m_rectDraw.top    = 0;
    pInfo->m_rectDraw.left   = 0;
    pInfo->m_rectDraw.right  = m_LogicalPageSize.cx;
    pInfo->m_rectDraw.bottom = m_LogicalPageSize.cy;

	DrawChart(pDC, &pInfo->m_rectDraw);

    // SetWindowOrg back for next page
    pDC->SetWindowOrg(0,0);

    pDC->SelectObject(pOldFont);
} 

void CChartCtrl::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    m_PrinterFont.DeleteObject();
	// RefreshChart is needed because the print job 
	// modifies the chart components (axis, ...)
	RefreshChart();
}

void CChartCtrl::GoToFirstSerie()
{
	m_currentSeries = m_mapSeries.begin();
}

CChartSerie* CChartCtrl::GetNextSerie()
{
	CChartSerie* pSeries = NULL;
	if (m_currentSeries != m_mapSeries.end())
	{
		pSeries = m_currentSeries->second;
		m_currentSeries++;
	}

	return pSeries;
}

#if _MFC_VER > 0x0600
void CChartCtrl::SaveAsImage(const TChartString& strFilename, 
							 const CRect& rect,
							 int nBPP,
							 REFGUID guidFileType)
{
	CImage chartImage;
	CRect chartRect = rect;
	if (chartRect.IsRectEmpty())
	{
		GetClientRect(&chartRect);
	}
    
	chartImage.Create(chartRect.Width(), chartRect.Height(), nBPP);
	CDC newDC;
	newDC.Attach(chartImage.GetDC());

	DrawBackground(&newDC, chartRect);
	chartRect.DeflateRect(3,3);
	DrawChart(&newDC, chartRect);

	newDC.Detach();
	chartImage.Save(strFilename.c_str(), guidFileType);
	chartImage.ReleaseDC();
}
#endif
