// MySerialPortDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MySerialPort.h"
#include "MySerialPortDlg.h"
#include "DataCheck.h"
#include "DataDecode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ECG_DATA   1
#define ECG_ACC_DATA   2
#define DATA_ANALYSIS_METHOD  ECG_ACC_DATA

#if DATA_ANALYSIS_METHOD == ECG_DATA
#define PACK_SIZE 20
#elif DATA_ANALYSIS_METHOD == ECG_ACC_DATA
#define PACK_SIZE 14
#endif

const UINT g_pack_head[2] = {0XAA, 0X55};

// CMySerialPortDlg 对话框




CMySerialPortDlg::CMySerialPortDlg(CWnd* pParent /*=NULL*/)
: CDialog(CMySerialPortDlg::IDD, pParent)
, m_bSerialPortOpened(false)
, m_nRecvCount(0)
, m_nRecvPack(0)
, m_nCorrectPack(0)
, m_index(0)
//, m_ar(&m_ecgfile,CArchive::store )
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMySerialPortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_COMPORT, m_ctrlComboComPort);
	DDX_Text(pDX, IDC_EDIT_COUNT, m_nRecvCount);
	DDX_Control(pDX, IDC_COMBO_COMPORT2, m_ctrlComboBaudRate);
	DDX_Control(pDX, IDC_COMBO_COMPORT3, m_ctrlComboData);
	DDX_Control(pDX, IDC_COMBO_COMPORT4, m_ctrlComboStop);
	DDX_Control(pDX, IDC_COMBO_COMPORT5, m_ctrlComboParity);
	DDX_Text(pDX, IDC_EDIT_COUNT2, m_nRecvPack);
	DDX_Text(pDX, IDC_EDIT_COUNT3, m_nCorrectPack);
	DDX_Text(pDX, IDC_EDIT1,	m_strPackData[0]);
	DDX_Text(pDX, IDC_EDIT2,	m_strPackData[1]);
	DDX_Text(pDX, IDC_EDIT3,	m_strPackData[2]);
	DDX_Text(pDX, IDC_EDIT4,	m_strPackData[3]);
	DDX_Text(pDX, IDC_EDIT5,	m_strPackData[4]);
	DDX_Control(pDX, IDC_TCHART1, m_Chart);
	//	DDX_Text(pDX, IDC_EDIT16, m_strAver[0]);	
	//	DDX_Text(pDX, IDC_EDIT17, m_strAver[1]);	
	//	DDX_Text(pDX, IDC_EDIT18, m_strAver[2]);
}

BEGIN_MESSAGE_MAP(CMySerialPortDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_COMM_RXCHAR, OnComm)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CMySerialPortDlg::OnButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CMySerialPortDlg::OnButtonClose)
	ON_BN_CLICKED(IDC_BUTTON_CLEARCOUNT, &CMySerialPortDlg::OnBnClickedButtonClearcount)
	ON_BN_CLICKED(IDC_BUTTON_CLEARDATA, &CMySerialPortDlg::OnBnClickedButtonCleardata)
	ON_EN_CHANGE(IDC_EDIT1, &CMySerialPortDlg::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CMySerialPortDlg 消息处理程序

BOOL CMySerialPortDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_ctrlComboComPort.SetCurSel(2);
	m_ctrlComboBaudRate.SetCurSel(0);
	m_ctrlComboData.SetCurSel(1);
	m_ctrlComboParity.SetCurSel(1);
	m_ctrlComboStop.SetCurSel(0);
	GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(!m_bSerialPortOpened);
	GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(m_bSerialPortOpened);
	for (int i=0;i<3;i++)
	{
		m_strPackData[i].Empty();

	}

	DWORD numElements[] = {1000};
	m_TValues.Create(VT_R8, 1, numElements);    
  m_YValues.Create(VT_R8, 1, numElements);
  m_ECGDataArray.SetSize(1000,20);
  m_XDataArray.SetSize(1000,20);
  m_YDataArray.SetSize(1000,20);
  m_ZDataArray.SetSize(1000,20);


	double dval = 0;
	long  i=0;
	for(i=0; i<1000; i++) 
	{
		dval = i;
		m_TValues.PutElement(&i, &dval);
		dval = m_XDataArray[i];
		m_YValues.PutElement(&i, &dval);
	};

	//m_MyXSeries = m_Chart.Series(0);
	m_myECG = m_Chart.Series(0);
	m_MyXSeries = m_Chart.Series(1);
	m_MyYSeries = m_Chart.Series(2);
	m_MyZSeries = m_Chart.Series(3);


	m_myECG.Clear();
	m_MyXSeries.Clear();
	m_MyYSeries.Clear();
	m_MyZSeries.Clear();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMySerialPortDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMySerialPortDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMySerialPortDlg::DrawSeries(int iSeriesIndex,const CArray<double,double> &myDataArray)
{
	CSeries mySeries = m_Chart.Series(iSeriesIndex);
	mySeries.Clear();
	double dval;
	long i;
	for(i=0; i<1000; i++) 
	{
		dval = myDataArray[i];
		m_YValues.PutElement(&i, &dval);
	}
	mySeries.AddArray(1000,m_YValues,m_TValues);
}
/*
double CMySerialPortDlg::GetWindowAver(int iSeriesIndex)
{
double fWindowAver=0;
for (int i =0;i<10;i++)
fWindowAver += m_fWindowData[iSeriesIndex][i];
fWindowAver/=10;
m_strAver[iSeriesIndex].Format("%0.3f",fWindowAver);
return fWindowAver;
}
*/
void CMySerialPortDlg::DataReady()
{
	static int s_serial_num = 0;
	static int s_pack_lost = 0;
	static int is_start = 0;

	unsigned int len = m_nPackData[0];

	//计算丢包，序列号是0-65535
	if( is_start == 0 )
	{
		is_start = 1;
		s_serial_num = m_nPackData[1];
	}
	else
	{
		s_serial_num++;
		if( s_serial_num ==  256 )
			s_serial_num = 0;

		int serial = m_nPackData[1];
		if( serial != s_serial_num )
		{
			int lost = serial-s_serial_num;
			s_pack_lost += lost;
			CTime tm=CTime::GetCurrentTime(); 
			CString str_time=tm.Format("%H-%M-%S");
			m_errfile.Write(str_time,str_time.GetLength());
			CString errstr;
			errstr.Format( "\r\n%d-%d %d packs lost this time, %d packs lost in all\r\n", s_serial_num, serial-1, lost, s_pack_lost );
			m_errfile.Write(errstr, strlen(errstr));

			s_serial_num = serial;
		}
	}

	//原始数据解析
	int n = 0;
	double ftmp = 0;
	int int32tmp = 0;
	unsigned int *p_data = &m_nPackData[2];

#if DATA_ANALYSIS_METHOD == ECG_DATA
	for ( int j=0; j != 5; j++ )
	{
		n = j*3;

		if( (p_data[n] == 0X000000A5) && (p_data[n+1] == 0X000000A5) && (p_data[n+2] == 0X000000A5) )
		{
      static int flag_num = 0;
			CString str="";
			str.Format("start flag %d\r\n", flag_num);
			m_ecgfile.Write(str, strlen(str));
      m_ecgcsfile.Write(str, strlen(str));
      flag_num++;
			continue;
		}

    if( p_data[n+2] == 0X000000FF )//压缩数据
    {
      p_data[n+1] = 0;
      p_data[n+2] = 0;
      //数据解码
      ftmp = DataDecode( &p_data[n], 3, "AD8232" );
      //保存数据
      CString str="";
      str.Format("%.4f\r\n",ftmp);
      m_ecgcsfile.Write(str, strlen(str));
    }
    else
    {
      //数据解码
      ftmp = DataDecode( &p_data[n], 3, "AD8232" );

      //保存数据
      CString str="";
      str.Format("%.4f\r\n",ftmp);
      m_ecgfile.Write(str, strlen(str));

      //画图
      m_strPackData[j].Format("%03f",ftmp);	
      m_XDataArray.RemoveAt(0,1);
      m_XDataArray.InsertAt(999,ftmp,1);
      DrawSeries(0,m_XDataArray);//画图
    }
		
	}
#elif DATA_ANALYSIS_METHOD == ECG_ACC_DATA    
	ftmp = DataDecode( &p_data[0], 3, "AD8232" );
	//保存数据
	CString str="";
	str.Format("%.4f\r\n",ftmp*500);
	m_ecgfile.Write(str, strlen(str));

	m_ECGDataArray.RemoveAt(0,1);
	m_ECGDataArray.InsertAt(999,ftmp*500,1);
	DrawSeries(0,m_ECGDataArray);

	ftmp = DataDecode( &p_data[3], 2, "ADXL345" );
	//保存数据
	str.Format("%.4f ",ftmp*500);
	m_accfile.Write(str, strlen(str));

	m_XDataArray.RemoveAt(0,1);
	m_XDataArray.InsertAt(999,ftmp*500,1);
	DrawSeries(1,m_XDataArray);

	ftmp = DataDecode( &p_data[5], 2, "ADXL345_1" );
	//保存数据
	str.Format("%.4f ",ftmp*500);
	m_accfile.Write(str, strlen(str));
	
	m_YDataArray.RemoveAt(0,1);
	m_YDataArray.InsertAt(999,ftmp*500,1);
	DrawSeries(2,m_YDataArray);

	ftmp = DataDecode( &p_data[7], 2, "ADXL345" );
	//保存数据
	str.Format("%.4f\r\n",ftmp*500);
	m_accfile.Write(str, strlen(str));

	m_ZDataArray.RemoveAt(0,1);
	m_ZDataArray.InsertAt(999,ftmp*500,1);
	DrawSeries(3,m_ZDataArray);

  ftmp = DataDecode( &p_data[9], 3, "AD8232" );
  //保存数据
  str.Format("%.4f\r\n",ftmp*3);
  m_votfile.Write(str, strlen(str));

#endif
	m_nCorrectPack ++;
}


LONG CMySerialPortDlg::OnComm(WPARAM ch, LPARAM port)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	/*	m_nPackData[m_index] = (UINT)ch;
	m_index ++;
	m_nRecvCount ++;
	if (m_index == PACK_SIZE)
	{
	DataReady();
	m_nRecvPack ++;
	m_index=0;
	UpdateData(FALSE);	
	}		*/
	m_nRecvCount++;
	static unsigned char pack_flag = 0;
	switch(pack_flag)
	{
	case 0X00:
		{
			if( (UINT)g_pack_head[0] == (UINT)ch )
				pack_flag |= 0X01;
		}
		break;
	case 0X01:
		{
			if( (UINT)g_pack_head[1] == (UINT)ch )
				pack_flag |= 0X02;
			else
				pack_flag = 0X00;
		}
		break;
	case 0X03:
		{
			m_nPackData[m_index] = (UINT)ch;
			m_index ++;
			if(m_index == m_nPackData[0])
			{
				if(1)//CheckData( m_nPackData, m_nPackData[0] ) == TRUE )
				{
					DataReady();
					m_nRecvPack ++;          
				}
				m_index=0;
				pack_flag = 0X00;
				UpdateData(FALSE);
			}
      else if( m_index  == PACK_MAX_SIZE )
      {
        m_index=0;
        pack_flag = 0X00;
      }
			break;
		}
	default:
		pack_flag = 0X00;
		break;
	}
	return 0;
}

void CMySerialPortDlg::OnButtonOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	int nPort=m_ctrlComboComPort.GetCurSel()+1;
	CString strBaudRate;
	CString strParity;
	CString strData;
	CString strStop;
	m_ctrlComboBaudRate.GetWindowText(strBaudRate);
	m_ctrlComboParity.GetWindowText(strParity);
	m_ctrlComboData.GetWindowText(strData);
	m_ctrlComboStop.GetWindowText(strStop);

	if (m_SerialPort.InitPort(this, nPort, atoi(strBaudRate),'N',atoi(strData),atoi(strStop),EV_RXFLAG|EV_RXCHAR,512))
	{
		m_SerialPort.StartMonitoring();
		m_bSerialPortOpened=TRUE;
		//m_ecgfile.Open("ecgdata.txt",CFile::modeCreate |CFile::modeNoTruncate | CFile::modeWrite );
		m_ecgfile.Open("ecgdata.txt",CFile::modeCreate | CFile::modeWrite );
		m_ecgfile.SeekToEnd();
		//m_errfile.Open("errdata.txt",CFile::modeCreate |CFile::modeNoTruncate | CFile::modeWrite );
		m_errfile.Open("errdata.txt",CFile::modeCreate | CFile::modeWrite );
		m_errfile.SeekToEnd();
		m_accfile.Open("accdata.txt",CFile::modeCreate | CFile::modeWrite );
		m_accfile.SeekToEnd();
    m_votfile.Open("votdata.txt",CFile::modeCreate | CFile::modeWrite );
    m_votfile.SeekToEnd();
    m_ecgcsfile.Open("ecgcsdata.txt",CFile::modeCreate | CFile::modeWrite );
    m_ecgcsfile.SeekToEnd();
	}
	else
	{
		AfxMessageBox("没有发现此串口或被占用");
		m_bSerialPortOpened=FALSE;
	}
	GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(!m_bSerialPortOpened);
	GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(m_bSerialPortOpened);
	UpdateData(TRUE);
}

void CMySerialPortDlg::OnButtonClose()
{
	// TODO: 在此添加控件通知处理程序代码
	m_SerialPort.ClosePort();//关闭串口
	m_index = 0;
	m_bSerialPortOpened=FALSE;
	//m_ar.Flush();
	m_ecgfile.Close();
	m_errfile.Close();
	m_accfile.Close();
  m_votfile.Close();
  m_ecgcsfile.Close();
	UpdateData(TRUE);
	GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(!m_bSerialPortOpened);
	GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(m_bSerialPortOpened);
}


void CMySerialPortDlg::OnBnClickedButtonClearcount()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nRecvCount=0;
	m_nCorrectPack=0;
	m_nRecvPack=0;
	UpdateData(FALSE);
}

void CMySerialPortDlg::OnBnClickedButtonCleardata()
{
	// TODO: 在此添加控件通知处理程序代码
	m_index = 0;

	for (int j=0;j<5;j++)
	{
		m_strPackData[j].Empty();
	}

	UpdateData(FALSE);
}

void CMySerialPortDlg::OnEnChangeEdit1()
{
	// TODO:  Èç¹û¸Ã¿Ø¼þÊÇ RICHEDIT ¿Ø¼þ£¬ÔòËü½«²»»á
	// ·¢ËÍ¸ÃÍ¨Öª£¬³ý·ÇÖØÐ´ CDialog::OnInitDialog()
	// º¯Êý²¢µ÷ÓÃ CRichEditCtrl().SetEventMask()£¬
	// Í¬Ê±½« ENM_CHANGE ±êÖ¾¡°»ò¡±ÔËËãµ½ÑÚÂëÖÐ¡£

	// TODO:  ÔÚ´ËÌí¼Ó¿Ø¼þÍ¨Öª´¦Àí³ÌÐò´úÂë
}
