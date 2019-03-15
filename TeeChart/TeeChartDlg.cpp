
// TeeChartDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "TeeChart.h"
#include "TeeChartDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
	srand((unsigned) time(NULL));//���������������
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTeeChartDlg �Ի���




CTeeChartDlg::CTeeChartDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTeeChartDlg::IDD, pParent)
	, m_nPointNum(16384)
	,m_c_arrayLength(10)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTeeChartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_nPointNum);
}

BEGIN_MESSAGE_MAP(CTeeChartDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTONDraw3, &CTeeChartDlg::OnBnClickedButtondraw3)
	ON_BN_CLICKED(IDC_BUTTON_Runing, &CTeeChartDlg::OnBnClickedButtonRuning)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_stop, &CTeeChartDlg::OnBnClickedButtonstop)
 
 
END_MESSAGE_MAP()


// CTeeChartDlg ��Ϣ�������

BOOL CTeeChartDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��
	CRect rect;
	// TODO: �ڴ���Ӷ���ĳ�ʼ������ 
	 
//	m_HSChartCtrl.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_CHART)->GetWindowRect(rect);
	ScreenToClient(rect);


	m_HSChartCtrl.Create(GetDlgItem(IDC_STATIC_CHART), rect, 2);


	UpdateData(FALSE);
	
	

	// Re enable the refresh
	m_HSChartCtrl.RefreshChart();



	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CTeeChartDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CTeeChartDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CTeeChartDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




double CTeeChartDlg::randf(double min,double max)
{
	int minInteger = (int)(min*10000);
	int maxInteger = (int)(max*10000);
	int randInteger = rand()*rand();
	int diffInteger = maxInteger - minInteger;
	int resultInteger = randInteger % diffInteger + minInteger;
	return resultInteger/10000.0;
}




void CTeeChartDlg::OnBnClickedButtondraw3()
{


	m_HSChartCtrl.RemoveAllSeries();

	
 

	srand((unsigned int)time(NULL));

	// Disable the refresh
	m_HSChartCtrl.EnableRefresh(false);
	COleDateTime Min(2008, 1, 26, 0, 0, 0);
	COleDateTime Max(2008, 9, 5, 0, 0, 0);
	// Create the bottom axis and configure it properly
	CChartDateTimeAxis* pBottomAxis = m_HSChartCtrl.CreateDateTimeAxis(CChartCtrl::BottomAxis);
	pBottomAxis->SetMinMax(Min, Max);
	pBottomAxis->SetDiscrete(true);
	pBottomAxis->SetTickIncrement(false, CChartDateTimeAxis::tiMonth, 1);
	pBottomAxis->SetTickLabelFormat(false, _T("%d %b"));
	// Create the left axis and configure it properly
	CChartStandardAxis* pLeftAxis = m_HSChartCtrl.CreateStandardAxis(CChartCtrl::LeftAxis);
	//pLeftAxis->SetMinMax(0, 100);
	pLeftAxis->SetFont(100, _T("Times New Roman"));
	pLeftAxis->GetLabel()->SetText(_T("Units sold"));
	pLeftAxis->SetAutomaticMode(CChartAxis::ScreenAutomatic);
	// Create the right axis and configure it properly
	CChartStandardAxis* pRightAxis = m_HSChartCtrl.CreateStandardAxis(CChartCtrl::RightAxis);
	pRightAxis->SetVisible(true);
	pRightAxis->GetLabel()->SetText(_T("Income (kEuros)"));
	pLeftAxis->SetAutomaticMode(CChartAxis::ScreenAutomatic);
	//pRightAxis->SetMinMax(0, 180);

	// Configure the legend
	m_HSChartCtrl.GetLegend()->SetVisible(true);
	m_HSChartCtrl.GetLegend()->SetHorizontalMode(true);
	//m_HSChartCtrl.GetLegend()->UndockLegend(80, 90);
	m_HSChartCtrl.GetLegend()->DockLegend(CChartLegend::dsDockBottom);
	// Add text to the title and set the font & color
	m_HSChartCtrl.GetTitle()->RemoveAll();

	m_HSChartCtrl.GetTitle()->AddString(_T("Income over 2008"));
	CChartFont titleFont;
	titleFont.SetFont(_T("Times New Roman"), 120, true, false, true);
	m_HSChartCtrl.GetTitle()->SetFont(titleFont);
	m_HSChartCtrl.GetTitle()->SetColor(RGB(0, 0, 128));
	// Sets a gradient background
	m_HSChartCtrl.SetBackGradient(RGB(255, 255, 255), RGB(150, 150, 255), gtVertical);

	// Create two bar series and a line series and populate them with data
	CChartBarSerie* pBarSeries1 = m_HSChartCtrl.CreateBarSerie();
	CChartBarSerie* pBarSeries2 = m_HSChartCtrl.CreateBarSerie();
	CChartLineSerie* pLineSeries = m_HSChartCtrl.CreateLineSerie();
	int lowIndex = -1;
	int lowVal = 999;
	for (int i = 0; i < 9; i++)
	{
		COleDateTime TimeVal(2008, i + 1, 1, 0, 0, 0);
		int DesktopVal = 20 + rand() % (100 - 30);
		pBarSeries1->AddPoint(TimeVal, DesktopVal);
		int LaptopVal = 10 + rand() % (80 - 20);
		pBarSeries2->AddPoint(TimeVal, LaptopVal);
		int Income = DesktopVal + LaptopVal*1.5;
		if (Income < lowVal)
		{
			lowVal = Income;
			lowIndex = i;
		}
		pLineSeries->AddPoint(TimeVal, Income);
	}
	// Configure the series properly
	
	pBarSeries1->SetName(_T("Desktops"));
	pBarSeries1->SetColor(RGB(255, 0, 0));
	pBarSeries1->SetBarWidth(1);
	pBarSeries1->EnableShadow(true);


	pBarSeries2->SetName(_T("Laptops"));
	pBarSeries2->SetColor(RGB(68, 68, 255));
	pBarSeries2->SetBarWidth(1);
	pBarSeries2->EnableShadow(true);
	//pBarSeries2->SetGradient(RGB(200, 200, 255), gtVertical);
	//pBarSeries2->SetBorderColor(RGB(0, 0, 255));


	pLineSeries->SetColor(RGB(0, 180, 0));
	pLineSeries->SetName(_T("Total income"));
	pLineSeries->SetWidth(1);
	//pLineSeries->EnableShadow(true);

	// Add a label on the line series.
	TChartStringStream labelStream;
	labelStream << _T("Min income: ") << lowVal;
	CChartBalloonLabel<SChartXYPoint>* pLabel =
		pLineSeries->CreateBalloonLabel(lowIndex, labelStream.str() + _T(" kEuros"));
	CChartFont labelFont;
	labelFont.SetFont(_T("Times New Roman"), 100, false, true, false);
	pLabel->SetFont(labelFont);

	m_HSChartCtrl.RefreshChart();

}




void CTeeChartDlg::OnBnClickedButtonstop()
{
	KillTimer(1);
	KillTimer(2);
}

void CTeeChartDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
 
	if (1==nIDEvent)
	{
		++m_count;
		drawMoving();
	}
 
	CDialogEx::OnTimer(nIDEvent);
}

void CTeeChartDlg::drawMoving()
{
	DWORD dwTimeS = GetTickCount();
	m_pLineSerie->ClearSerie();
	RandArray(m_HightSpeedChartArray,m_c_arrayLength);
	LeftMoveArray(m_X,m_c_arrayLength,m_count);
	m_pLineSerie->AddPoints(m_X,m_HightSpeedChartArray,m_c_arrayLength);
	m_HSChartCtrl.RefreshChart();
		DWORD dwTimeE = GetTickCount();

	CString str;
	str.Format(_T("��ʱ��%d ms"), dwTimeE - dwTimeS);
	SetDlgItemText(IDC_STATIC_T, str);
}
/// 
/// \brief ��������
/// \param ptr ����ָ��
/// \param data ����ֵ
///
void CTeeChartDlg::LeftMoveArray(double* ptr,size_t length,double data)
{
	for (size_t i=1;i<length;++i)
	{
		ptr[i-1] = ptr[i];
	}
	ptr[length-1] = data;
}

void CTeeChartDlg::RandArray(double* ptr,size_t length)
{
	for (size_t i=0;i<length;++i)
	{
		ptr[i] = randf(0,1000);
	}
}


void CTeeChartDlg::OnBnClickedButtonRuning()
{
	UpdateData(TRUE);
	if (m_nPointNum <= 0)
	{
		MessageBox(_T("���ݵ�������С��0"));
		return;
	}

	KillTimer(1);
	CChartAxis *pAxis = NULL;
	//��ʼ������

	//pAxis->SetAutomatic(true);
	pAxis = m_HSChartCtrl.CreateStandardAxis(CChartCtrl::LeftAxis);
	pAxis->SetFont(100, _T("Times New Roman"));
	//pAxis->SetMinMax(0, 35);
	pAxis->SetAutomatic(true);

	pAxis = m_HSChartCtrl.CreateStandardAxis(CChartCtrl::BottomAxis);
	pAxis->SetFont(100, _T("Times New Roman"));
	pAxis->SetAutomatic(true);

	m_HSChartCtrl.RemoveAllSeries();
	m_pLineSerie = m_HSChartCtrl.CreateLineSerie();
	ZeroMemory(&m_HightSpeedChartArray, sizeof(double)*m_c_arrayLength);
	for (size_t i = 0; i<m_c_arrayLength; ++i)
	{
		m_X[i] = i;
	}
	m_count = m_c_arrayLength;
	m_pLineSerie->ClearSerie();
	SetTimer(1, 1000, NULL);	//��ˢ��Ƶ������Ϊ0�ͻ�ȫ��������
}


 


