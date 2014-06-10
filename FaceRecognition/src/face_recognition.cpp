/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "pxcsession.h"
#include "pxccapture.h"
#include "pxcsmartptr.h"
#include "face_render.h"
#include "util_capture_file.h"
#include "util_cmdline.h"
#include "pxcface.h"
#include <conio.h>
#include <fstream>
#include <ctime>
#include <stdlib.h>
#include <string>
#include <stack>

using namespace std;

const pxcU32 MAX_DB_SIZE =100;
PXCFaceAnalysis::Recognition::Model *models[MAX_DB_SIZE];
pxcF32 scores[MAX_DB_SIZE];
pxcCHAR nameDB[MAX_DB_SIZE][64];
pxcU32  nmodels = 0;


int wmain(int argc, WCHAR* argv[]) {   
	// create a file
	std::ofstream ofs;
	std::ifstream ifs;


    // Create a session
    PXCSmartPtr<PXCSession> session;
    pxcStatus sts=PXCSession_Create(&session);
	if (sts<PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to create the SDK session\n");
		return 3;
	}

    UtilCmdLine cmdl(session);
    if (!cmdl.Parse(L"-sdname-nframes-file-record",argc,argv)) return 3;  

    // Init Face analyzer
    PXCSmartPtr<PXCFaceAnalysis> faceAnalyzer;
    sts=session->CreateImpl<PXCFaceAnalysis>(cmdl.m_iuid, &faceAnalyzer);
	if (sts<PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to locate a face analysis module\n");
		return 3;
	}

	// Retrieve input requirements
    PXCFaceAnalysis::ProfileInfo faInfo;
    faceAnalyzer->QueryProfile(0, &faInfo);
	
    // Find capture device
    UtilCaptureFile capture(session,cmdl.m_recordedFile,cmdl.m_bRecord);
    if (cmdl.m_sdname) capture.SetFilter(cmdl.m_sdname); /*L"Integrated Camera"*/
    sts=capture.LocateStreams(&faInfo.inputs);
	if (sts<PXC_STATUS_NO_ERROR) {
		wprintf_s(L"Failed to locate an input device suitable for face recognition\n");
		return 3;
	}
    faceAnalyzer->SetProfile(&faInfo);
   
    // Set recognition profile
    PXCFaceAnalysis::Recognition::ProfileInfo rInfo={0};
    PXCFaceAnalysis::Recognition *faceRecognizer=faceAnalyzer->DynamicCast<PXCFaceAnalysis::Recognition>();
	if (!faceRecognizer) {
		wprintf_s(L"Failed to locate the face recognition interface\n");
		return 3;
	}

    faceRecognizer->QueryProfile(0, &rInfo);
    faceRecognizer->SetProfile(&rInfo);
               
    // Create Renderer
    PXCSmartPtr<FaceRender> faceRender(new FaceRender(L"Face Recognition Sample"));
    for (int fnum=0;fnum<(int)cmdl.m_nframes;fnum++) {
        PXCSmartArray<PXCImage> images;
        PXCSmartSPArray sps(2);

        // read and process frame
        sts = capture.ReadStreamAsync(images,&sps[0]);
        if (sts<PXC_STATUS_NO_ERROR) break; // EOF
        
        sts=faceAnalyzer->ProcessImageAsync(images, &sps[1]);
		if (sts<PXC_STATUS_NO_ERROR) break;

        sts = sps.SynchronizeEx();
        if (sps[0]->Synchronize(0)<PXC_STATUS_NO_ERROR) break; // EOF
       
        // loop all faces
        faceRender->ClearData();


		// create a file
		stack<string> st, st2;

		string s;
		ifs.open("detectedFaces");
		while(!ifs.eof()){
			std::getline(ifs, s);
			st.push(s);
		}
		ifs.close();

		int ii = 0;
		while(!st.empty() && (ii < 100)){
			st2.push(st.top());
			st.pop();
			ii++;
		}

		ofs.open("detectedFaces");
		while(!st2.empty()){
			ofs << st2.top() << std::endl;
			st2.pop();
		}

		ofs << time(0) << ", ";
        for (int fidx = 0; ; fidx++) {
            pxcUID fid = 0;
            pxcU64 timeStamp = 0;
            sts = faceAnalyzer->QueryFace(fidx, &fid, &timeStamp);

            if (sts < PXC_STATUS_NO_ERROR) break; //no more faces
			
            if(nmodels) {
                //CreateModel for current face            
                PXCFaceAnalysis::Recognition::Model *curModel=0;
                sts = faceRecognizer->CreateModel(fid, &curModel);
                if (sts < PXC_STATUS_NO_ERROR) break; //cannot create Model.

                //Compare current face model to all available DB models;
                pxcU32 index;
                sts = curModel->Compare(models, nmodels, scores, &index);
                if(sts>=PXC_STATUS_NO_ERROR){
                    faceRender->SetRecognitionData(faceAnalyzer, nameDB[index], _countof(nameDB[index]), fid);
					//wprintf_s(L"%s,", &nameDB[index]);
					wstring ss = wstring(nameDB[index]);
					ofs << string(ss.begin(), ss.end()) << ", ";
				}


            }

            //Populate face Database
            if (_kbhit() && nmodels<MAX_DB_SIZE) {
                int key = _getch();
				if(key == 'a'|| key == 'A') {   //get name from user
                    wprintf_s(L">>> Type the name of the person detected:\n");
                    wscanf_s(L"%63[^\r\n]s\n", nameDB[nmodels], _countof(nameDB[nmodels]));
                    fflush(stdin);
                    if(wcslen(nameDB[nmodels]) > 0){
                        sts = faceRecognizer->CreateModel(fid, &models[nmodels]);
                        if (sts < PXC_STATUS_NO_ERROR) {
                            wprintf_s(L">>> Could Not create Model\n");
                            wprintf_s(L">>> Press 'a' to add current picture to record\n");
                            break; //cannot create Model.
                        }
                        nmodels++;
                    }
                    else wprintf_s(L">>> Incorrect name\n");
                    wprintf_s(L">>> Press 'a' to add current picture to record\n");
                }
            }
        }
        //ofs << std::endl;
		ofs.close();
        /* Setting a specific number of frames to run */
        if (!faceRender->RenderFrame(images[0])) break;
    }
   
    return(0);
}