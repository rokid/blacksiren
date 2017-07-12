//
//  r2mem_rdc.cpp
//  r2ad2
//
//  Created by hadoop on 11/4/16.
//  Copyright © 2016 hadoop. All rights reserved.
//

#include "legacy/r2mem_rdc.h"

r2mem_rdc::r2mem_rdc(r2_mic_info* pMicInfo_Rdc, r2_mic_info* pMicInfo_AecRef, int iMaxLen) {

    m_pMicInfo_Rdc = pMicInfo_Rdc ;
    m_pMicInfo_AecRef = pMicInfo_AecRef ;
    m_iCurLen = 0 ;
    m_iMaxLen = iMaxLen ;

    m_pRDc = R2_SAFE_NEW_AR1(m_pRDc, float, m_pMicInfo_Rdc->iMicNum);
    m_pRDc_Var = R2_SAFE_NEW_AR1(m_pRDc_Var, double, m_pMicInfo_Rdc->iMicNum);
    m_bMicOk = R2_SAFE_NEW_AR1(m_bMicOk, int, m_pMicInfo_Rdc->iMicNum);
    for (int i = 0 ; i < m_pMicInfo_Rdc->iMicNum ; i ++) {
        m_bMicOk[i] = 1 ;
    }
}

r2mem_rdc::~r2mem_rdc(void) {

    R2_SAFE_DEL_AR1(m_pRDc) ;
    R2_SAFE_DEL_AR1(m_pRDc_Var) ;
    R2_SAFE_DEL_AR1(m_bMicOk) ;
}

int r2mem_rdc::reset() {

    memset(m_pRDc, 0, m_pMicInfo_Rdc->iMicNum * sizeof(float));
    memset(m_pRDc_Var, 0, m_pMicInfo_Rdc->iMicNum * sizeof(double));
    m_iCurLen = 0 ;

    return 0 ;
}

int r2mem_rdc::process(float** pData_In, int& iLen_In) {

    int iCur = r2_min(m_iMaxLen - m_iCurLen, iLen_In) ;


    if (m_iCurLen < m_iMaxLen) {
        //Skip Aec Data
#if 0
        for (int i = 0 ; i < m_pMicInfo_AecRef->iMicNum ; i ++) {
            int iMicId = m_pMicInfo_AecRef->pMicIdLst[i];
            for (int j = 0 ; j < iLen_In ; j ++) {
                if (pData_In[iMicId][j] != 0.0f) {
                    iLen_In = 0 ;
                    reset();
                    return 0 ;
                }
            }
        }
#endif

        for (int i = 0 ; i < m_pMicInfo_Rdc->iMicNum ; i ++) {
            int iMicId = m_pMicInfo_Rdc->pMicIdLst[i];
            for (int j = 0 ; j < iCur ; j ++) {
                m_pRDc[i] += pData_In[iMicId][j] ;
                m_pRDc_Var[i] += pData_In[iMicId][j] * pData_In[iMicId][j] ;
            }
        }

        m_iCurLen += iCur ;

        if (m_iCurLen == m_iMaxLen) {
            for (int i = 0 ; i < m_pMicInfo_Rdc->iMicNum ; i ++) {
                m_pRDc[i] = m_pRDc[i] / m_iCurLen ;
                m_pRDc_Var[i] = sqrt(m_pRDc_Var[i] / m_iCurLen - (m_pRDc[i] * m_pRDc[i]) + 0.1f) ;
                ZLOG_INFO("----Calculate Mic AVG %d: %f", m_pMicInfo_Rdc->pMicIdLst[i], m_pRDc_Var[i]);
            }
            for (int i = 0 ; i < m_pMicInfo_Rdc->iMicNum ; i ++) {
                if (m_pRDc_Var[i] > 200000) {
                    ZLOG_INFO("-----Mic Error %d:-------------", m_pMicInfo_Rdc->pMicIdLst[i]);
                    m_bMicOk[i] = 0 ;
                } else {
                    m_bMicOk[i] = 1 ;
                }
            }
        }

    }

    iLen_In = iLen_In - iCur ;
    if (iLen_In > 0) {
        for (int i = 0 ; i < m_pMicInfo_Rdc->iMicNum ; i ++) {
            int iMicId = m_pMicInfo_Rdc->pMicIdLst[i];
            if (m_bMicOk[i] == 0) {
                memset(pData_In[iMicId], 0, sizeof(float) * iLen_In);
            } else {
                for (int j = 0 ; j < iLen_In ; j ++) {
                    pData_In[iMicId][j] = pData_In[iMicId][j + iCur] -  m_pRDc[i] ;
                }
            }
        }
    }


    return 0 ;
}



