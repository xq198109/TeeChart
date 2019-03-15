/*
 *
 *	ChartAxis.cpp
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
 */

#include "stdafx.h"
#include "ChartStandardAxis.h"
#include "ChartCtrl.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include "math.h"

using namespace std;

CChartStandardAxis::CChartStandardAxis()
 : CChartAxis(),  m_uDecCount(0)	
{
}

CChartStandardAxis::~CChartStandardAxis()
{
}


void CChartStandardAxis::SetTickIncrement(bool bAuto, double newIncrement)		
{ 
	m_bAutoTicks = bAuto;
	if (!m_bAutoTicks)
	{
		m_dTickIncrement = newIncrement; 
	
		int Zeros = (int)floor(log10(m_dTickIncrement));

		int Digits = 0;
		if (Zeros<0)		
		{
			//We must set decimal places. In the other cases, Digits will be 0.
			Digits = (int)fabs(Zeros*1.0);
		}
		SetDecimals(Digits);
	}
}

double CChartStandardAxis::GetFirstTickValue() const
{ 
	double dRetVal = m_dFirstTickValue;
	//if (m_bDiscrete)
	//{
	//	dRetVal = m_dFirstTickValue - m_dTickIncrement;
	//}
	return dRetVal; 
}

bool CChartStandardAxis::GetNextTickValue(double dCurrentTick, double& dNextTick) const
{

	if (m_dTickIncrement == 0)
		return false;

	dNextTick = dCurrentTick + m_dTickIncrement;
	 
	if (m_dLastTickValue- dNextTick >= -0.0000001)
		return true;
	else
		return false;

	
}

double CChartStandardAxis::ScreenToValue(long ScreenVal) const
{

	if (m_MaxValue == m_MinValue)
		return m_MinValue;

	int AxisOffset = 0;
	if (!m_bIsHorizontal)
	{
		if (m_bIsInverted)
			AxisOffset = ScreenVal - m_EndPos;
		else
			AxisOffset = m_StartPos - ScreenVal;
	}
	else
	{
		if (!m_bIsInverted)
			AxisOffset = ScreenVal - m_StartPos;
		else
			AxisOffset = m_EndPos - ScreenVal;
	}

	double retVal;

	if (!m_bDiscrete)
	{

		retVal = ((AxisOffset * 1.0 / GetAxisLenght()*(m_MaxValue - m_MinValue)) + m_MinValue);
	}
	else
	{
		retVal = ((AxisOffset * 1.0 / GetAxisLenght()*(m_dLastTickValue - m_dFirstTickValue + m_dTickIncrement)) + m_dFirstTickValue);
		retVal = floor(retVal / m_dTickIncrement)* m_dTickIncrement;

	}
	return retVal;
}




long CChartStandardAxis::ValueToScreenDiscrete(double dValue) const
{
	// In discrete mode, all values between two ticks are "directed"
	// to the middle of the interval.
	//double precision = 0.0000000001;
	//if (dValue < 0)
	//	precision = -0.0000000001;
	//int MintickNr = floor(m_MinValue / m_dTickIncrement);
	//int MaxtickNr = floor(m_MaxValue / m_dTickIncrement);
	//int tickNr = floor(dValue / m_dTickIncrement);
	//dValue = tickNr * m_dTickIncrement;

	//dValue += m_dTickIncrement/2.0;
	//return ValueToScreenStandard(dValue);

	//double dDiscreteValue = floor(dValue/ m_dTickIncrement)* m_dTickIncrement + m_dTickIncrement / 2.0;

	long Offset = 0;
	long retVal = 0;
	if (dValue - floor(dValue / m_dTickIncrement)* m_dTickIncrement > 0.9999999)
		dValue = (dValue / m_dTickIncrement)* m_dTickIncrement + m_dTickIncrement*1.5;
	else
		dValue = (dValue / m_dTickIncrement)* m_dTickIncrement + m_dTickIncrement*0.5;

	Offset = floor((dValue - m_dFirstTickValue) * GetAxisLenght() / (m_dLastTickValue - m_dFirstTickValue + m_dTickIncrement));
	if (m_bIsHorizontal)
	{
		if (!m_bIsInverted)
			retVal = (m_StartPos + Offset);
		else
			retVal = (m_EndPos - Offset);
	}
	else
	{
		if (!m_bIsInverted)
			retVal = (m_StartPos - Offset);
		else
			retVal = (m_EndPos + Offset);
	}
	return retVal;
}

long CChartStandardAxis::GetTickPos(double TickVal) const
{
	// The tick is always at the same position,
	// even if the axis is discrete.

	double precision = 0.0000000001;
	if (TickVal < 0)
		precision = -0.0000000001;

	long Offset = 0;
	long retVal = 0;

	if (m_bIsHorizontal)
	{
		if (m_bDiscrete)
		{
			//double MintickValue = ((int)((m_MinValue + precision) / m_dTickIncrement))*m_dTickIncrement;
			//double MaxtickValue = ((int)((m_MaxValue + precision) / m_dTickIncrement) + 1)*m_dTickIncrement;
			Offset = (int)floor((TickVal - m_dFirstTickValue) * GetAxisLenght() / (m_dLastTickValue - m_dFirstTickValue + m_dTickIncrement));
		}
		else
			Offset = (int)floor((TickVal - m_MinValue) * GetAxisLenght() / (m_MaxValue - m_MinValue));
	
		if (!m_bIsInverted)
			retVal = (m_StartPos + Offset);
		else
			retVal = (m_EndPos - Offset);
	}
	else
	{
		if (m_bDiscrete)
		{
			//double MintickValue = ((int)((m_MinValue + precision) / m_dTickIncrement))*m_dTickIncrement;
			//double MaxtickValue = ((int)((m_MaxValue + precision) / m_dTickIncrement) + 1)*m_dTickIncrement;
			Offset = (int)floor((TickVal - m_dFirstTickValue) * GetAxisLenght() / (m_dLastTickValue - m_dFirstTickValue + m_dTickIncrement));
		}
		else
			Offset = (int)floor((TickVal - m_MinValue) * GetAxisLenght() / (m_MaxValue - m_MinValue));


		if (!m_bIsInverted)
			retVal = (m_StartPos - Offset);
		else
			retVal = (m_EndPos + Offset);
	}
	return retVal;
}

TChartString CChartStandardAxis::GetTickLabel(double TickValue) const
{
	TChartStringStream ssLabel;
	ssLabel << fixed << setprecision(m_uDecCount) << TickValue;
	return ssLabel.str();
}

void CChartStandardAxis::RefreshTickIncrement(CDC* pDC)
{
	if (!m_bAutoTicks)
		return;

	if (m_MaxValue == m_MinValue)
	{
		m_dTickIncrement = 0;
		return;
	}

	int PixelSpace;
	if (m_bIsHorizontal)
	{
		CSize TextSize = GetLargestTick(pDC);
		if (TextSize.cx < 20)
			PixelSpace = 30;
		else
			PixelSpace = 10 + TextSize.cx;

	}
	
	else
		PixelSpace = 20;

	int MaxTickNumber = (int)fabs((m_EndPos-m_StartPos)/PixelSpace * 1.0);

	//Calculate the appropriate TickSpace (1 tick every 30 pixel +/-)每个刻度和刻度值的区域占30像素
	//Temporary tick increment最大刻度数
	double TempTickIncrement = (m_MaxValue-m_MinValue)/MaxTickNumber;//计算最小刻度值

	// Calculate appropriate tickSpace (not rounded on 'strange values' but 
	// on something like 1, 2 or 5*10^X  where X is optimalized for showing the most
	// significant digits)计算以10为倍数最小刻度值（1, 10，100，1000，....）（0.1, 0.01，0.0001，....）
	int Zeros = (int)ceil(log10(TempTickIncrement));
	double MinTickIncrement = pow(10,Zeros);
	//最小刻度值小于10时
	int Digits = 0;
	if (Zeros<0)		
	{
		//We must set decimal places. In the other cases, Digits will be 0.
		Digits = (int)fabs(Zeros*1.0);
	}

	if (MinTickIncrement>=TempTickIncrement)
	{
		//最小刻度等于1，10，1000
		m_dTickIncrement = MinTickIncrement;
		SetDecimals(Digits);
	}
	else if (MinTickIncrement*2>=TempTickIncrement)
	{
		//取2倍最小刻度
		m_dTickIncrement = MinTickIncrement*2;
		SetDecimals(Digits);
	}
	else if (MinTickIncrement*5>=TempTickIncrement)
	{
		//取5倍最小刻度
		m_dTickIncrement = MinTickIncrement*5;
		SetDecimals(Digits);
	}
	else if (MinTickIncrement*10>=TempTickIncrement)
	{
		//超过5倍整刻度刻度，取下一级整刻度
		m_dTickIncrement = MinTickIncrement*10;
		if (Digits)
			SetDecimals(Digits-1);
		else
			SetDecimals(Digits);
	}
}

void CChartStandardAxis::RefreshFirstTick()
{
	if (m_dTickIncrement!=0)
	{
		if (m_bDiscrete)
			m_dFirstTickValue = floor(m_MinValue / m_dTickIncrement) * m_dTickIncrement;
		else
		{
			//if (m_bIsHorizontal)
			//{

			//	int  n = ceil(m_MinValue / m_dTickIncrement);
			//	m_dFirstTickValue = n* m_dTickIncrement;
			//}
			//else
			//{
			//	m_dFirstTickValue = (floor(m_MinValue / m_dTickIncrement) + 1)* m_dTickIncrement;
			// 

			//}
				int  n = ceil(m_MinValue / m_dTickIncrement);
				m_dFirstTickValue = n* m_dTickIncrement;
		
		}

		 
	}
	else	// m_TickIncrement!=0
	{
		m_dFirstTickValue = m_MinValue;
	}


	if (m_dTickIncrement != 0)
	{
		//if (m_bIsHorizontal)
		//	m_dLastTickValue = floor(m_MaxValue / m_dTickIncrement)* m_dTickIncrement;
		//else
		//{
		//	int  n = ceil(m_MaxValue / m_dTickIncrement);
		//	m_dLastTickValue = (n - 1) * m_dTickIncrement;
		//}
		int  n = (m_MaxValue - m_dFirstTickValue)/ m_dTickIncrement;
		m_dLastTickValue = n* m_dTickIncrement + m_dFirstTickValue;
	}
	else	// m_TickIncrement!=0
	{
		m_dLastTickValue = m_MaxValue;
	}



}
