

/*
 * Copyright (C) 2018 Fuzhou Rockchip Electronics Co.Ltd.
 *
 * Modification based on code covered by the Apache License, Version 2.0 (the "License").
 * You may not use this software except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS TO YOU ON AN "AS IS" BASIS
 * AND ANY AND ALL WARRANTIES AND REPRESENTATIONS WITH RESPECT TO SUCH SOFTWARE, WHETHER EXPRESS,
 * IMPLIED, STATUTORY OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF TITLE,
 * NON-INFRINGEMENT, MERCHANTABILITY, SATISFACTROY QUALITY, ACCURACY OR FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.
 *
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <paintworker.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cutils/properties.h>

#include <linux/input.h>
#include "getevent.h"
#include "wenote_jni.h"
//open header
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//map header
#include <map>
#include<math.h>

#include <time.h>

#include <utils/Log.h>
#include <utils/Timers.h>


using namespace android;
using namespace std;
#include <vector>




static unsigned int  x     = 0;
static unsigned int  y     = 0;
static unsigned int  lastX = 0;
static unsigned int  lastY = 0;
static unsigned int  RelastX = 0;
static unsigned int  RelastY = 0;

static SkBitmap bitmap;
static SkCanvas *canvas;
static SkPaint paint;
static float XScale = 0.089;//0.999;//0.089;//0.059;//depend on pen device
static float YScale = 0.089;//0.999;//0.089;//0.059;

static float mXScale = 0.999;//depend on device
static float mYScale = 0.998;

//手写区域
static int win_x1, win_y1, win_x2, win_y2;

static bool isPressed = false; // pen isPressed
static bool isPen = false;//is a pen
static bool startDrawPoint = false;


static int penMode = 0;
static int m_pressed_value;

static int mPenColor = 1;//画笔颜色
static int mPenWidth = 4;//画笔宽度
static int mEraserColor = 2;//橡皮擦颜色
static int mEraserWidth = 12;//橡皮擦宽度


typedef struct {
	int x;
	int y;
	int m_pressed_value;
	int penColor;
	int penWidth;
	int action;
    } PointStruct;
static vector <PointStruct> pathVector;
static vector <vector<PointStruct>> noteVector;

PaintWorker::PaintWorker()
    : Worker("PaintWorker", HAL_PRIORITY_URGENT_DISPLAY) {
	fprintf(stderr, "new PaintWorker\n");

}
PaintWorker::~PaintWorker(){
	fprintf(stderr, "delete PaintWorker\n");
	if(rgba_buffer != NULL){
		free(rgba_buffer);
		rgba_buffer = NULL;
	}
	uninit_getevent();
}


int PaintWorker::Init(int left, int top, int right, int bottom){

  ALOGI("PaintThread struct \n");

  pen_width = 4;
  pen_old_width = 4;

  paint.setARGB(255, 0, 0, 0);
  paint.setStyle(SkPaint::kStroke_Style);
  //paint.setAntiAlias(true);
  paint.setStrokeWidth(pen_width);
  paint.setStrokeCap(SkPaint::kRound_Cap); //kRound_Cap
  pen_color = PEN_BLACK_COLOR;
  pen_old_color = PEN_BLACK_COLOR;

  if (init_getevent() != 0) {
      ALOGD("error: PaintWorker did not work!\n");
  }


  int fd = open("/dev/ebc", O_RDWR,0);
  if (fd < 0){
      ALOGE("open /dev/ebc failed\n");
  }

  struct ebc_buf_info_t ebc_buf_info;

  if(ioctl(fd, EBC_GET_BUFFER_INFO,&ebc_buf_info)!=0){
      ALOGE("GET_EBC_BUFFER failed\n");
  }
#ifdef SKIA_DRAW_RGB_IMAGE
  rgba_buffer = (void *)malloc(ebc_buf_info.width * ebc_buf_info.height * 4);

  memset(rgba_buffer,0xff,ebc_buf_info.width * ebc_buf_info.height * 4);
  bitmap.allocN32Pixels(ebc_buf_info.width, ebc_buf_info.height);
  bitmap.setPixels((void *)rgba_buffer);
#else
  rgba_buffer = (void *)malloc(ebc_buf_info.width * ebc_buf_info.height);

  memset(rgba_buffer,0xff,ebc_buf_info.width * ebc_buf_info.height);
  SkImageInfo info;
  info = SkImageInfo::Make(ebc_buf_info.width, ebc_buf_info.height, kGray_8_SkColorType,  kOpaque_SkAlphaType); //
  bitmap.installPixels(info, (void *)rgba_buffer, ebc_buf_info.width);
#endif

  close(fd);

  commitWorker.Init();

  commitWorker.set_path_buffer(rgba_buffer);
  commitWorker.setPenWidth(pen_width);
  canvas = new SkCanvas(bitmap);
  if(NULL != canvas )
	  ALOGD("---canvas exist\n");
  //win_x1 = 0;
  //win_y1 = 0;
  //win_x2 = ebc_buf_info.width;
  //win_y2 = ebc_buf_info.height;
  win_x1 = top;
  win_y1 = right;
  win_x2 = bottom;
  win_y2 = left;
 
  commitWorker.init_draw_win_info(win_x1, win_y1, win_x2, win_y2);

  return InitWorker();
}
void PaintWorker::onFirstRef() {
}
void *PaintWorker::Get_path_buffer(){
    return rgba_buffer!= NULL ? rgba_buffer : NULL;
}
void PaintWorker::ExitPaintWorker(){
    commitWorker.Exit();
    Exit();
}

void PaintWorker::ReDrawPoint(int x, int y, int action){
	ALOGD("---start ReDraw -lastX=%d--lastY=%d--x=%d--y=%d--action=%d \n", RelastX, RelastY, x, y, action);	
	if(action == 0 && !startDrawPoint) {
		ALOGD("---set startDrawPoint is true -lastX=%d--lastY=%d--x=%d--y=%d--action=%d \n", RelastX, RelastY, x, y, action);
		startDrawPoint = true;
	} else if(action == 2 && startDrawPoint && x != 0 && y!= 0) {
		ALOGD("---start RedrawPoint -lastX=%d--lastY=%d--x=%d--y=%d--action=%d \n", RelastX, RelastY, x, y, action);
        canvas->drawPoint(x, y, paint);
		RelastX = x;
		RelastY = y;
		startDrawPoint = false;
	} else if (action == 2 && !startDrawPoint && RelastX != 0 && RelastY != 0 && x != 0 && y!= 0) {
		ALOGD("---start RedrawLine -lastX=%d--lastY=%d--x=%d--y=%d--action=%d \n", RelastX, RelastY, x, y, action);
		canvas->drawLine(RelastX, RelastY, x, y, paint);
		RelastX = x;
		RelastY = y;
	}
	//usleep(1000000);
	//commitWorker.set_win_info(x,y);
	//commitWorker.Signal();
}

void PaintWorker::ShowDrawPoint(int left, int top, int right, int bottom){
	commitWorker.init_draw_win_info(left, top, right, bottom);
    commitWorker.Signal();
}

void PaintWorker::ShowReDraw(int x, int y){
	commitWorker.set_win_info(x,y);
    commitWorker.Signal();
}


int PaintWorker::Undo(int left, int top, int right, int bottom){
	if(noteVector.size() > 0) {
		clearScreen(0);
		noteVector.pop_back();
		ALOGD("----Undo--noteVector.size:%d \n", noteVector.size());
		vector<vector<PointStruct>>::iterator itPath;
		vector<PointStruct>::iterator itPoint;
		vector<PointStruct> vec_tmp;
		//GetPenConfig(&paint);
		//paint.setStrokeWidth(10);
		for (itPath = noteVector.begin(); itPath != noteVector.end(); itPath++)
			{
				vec_tmp = *itPath;
				for (itPoint = vec_tmp.begin(); itPoint != vec_tmp.end(); itPoint++)
					{
						ALOGD("----Undo--x:%d,y:%d,m_pressed_value:%d,penColor:%d,penWidth:%d,action:%d \n", 
							itPoint->x, itPoint->y, itPoint->m_pressed_value, itPoint->penColor, itPoint->penWidth, itPoint->action);
						pen_color = itPoint->penColor;
						pen_width = itPoint->penWidth;
						GetPenConfig(&paint);
						ReDrawPoint(itPoint->x, itPoint->y, itPoint->action);
					}
			}
		ShowDrawPoint(left, top, right, bottom);
		SetPenOrEraserFromMode();
		return 1;
	} else {
		ALOGD("----noteVector.size <= 0");
		return 0;
	}
}

int PaintWorker::ReDraw(int left, int top, int right, int bottom) {
	if(noteVector.size() > 0) {
		clearScreen(0);
		ALOGD("----ReDraw--noteVector.size:%d \n", noteVector.size());
		vector<vector<PointStruct>>::iterator itPath;
		vector<PointStruct>::iterator itPoint;
		vector<PointStruct> vec_tmp;
		//GetPenConfig(&paint);
		//paint.setStrokeWidth(10);
		for (itPath = noteVector.begin(); itPath != noteVector.end(); itPath++)
			{
				vec_tmp = *itPath;
				for (itPoint = vec_tmp.begin(); itPoint != vec_tmp.end(); itPoint++)
					{
						ALOGD("----ReDraw--x:%d,y:%d,m_pressed_value:%d,penColor:%d,penWidth:%d,action:%d \n", 
							itPoint->x, itPoint->y, itPoint->m_pressed_value, itPoint->penColor, itPoint->penWidth, itPoint->action);
						pen_color = itPoint->penColor;
						pen_width = itPoint->penWidth;
						GetPenConfig(&paint);
						ReDrawPoint(itPoint->x, itPoint->y, itPoint->action);
						usleep(10);
						ShowReDraw(itPoint->x, itPoint->y);
					}
			}
		SetPenOrEraserFromMode();
		return 1;
	} else {
		ALOGD("----ReDraw--noteVector.size <= 0");
		return 0;
	}
}


void PaintWorker::Routine() {
	input_event e;
	
	//nsecs_t start = systemTime(SYSTEM_TIME_MONOTONIC);

	//SkPath path;
	if(isDrawing == 1) {
		return;
	}

	if (get_event(&e, 500) == 0) {
		//ALOGD("---e.value*mYScale:%d,win_x1:%d,win_x2:%d,win_y1:%d,win_y2:%d---", e.value*mYScale,win_x1, win_x2, win_y1, win_y2);
			
		//long te = static_cast<long>(e.time.tv_sec) * 1000000 + e.time.tv_usec;
		/*
		*  read axis (x, y)
		*/
		if (e.type == EV_ABS && e.code == ABS_PRESSURE) {
			ALOGD("MyFlash test : ++++++++ press value = %d\n", e.value);
			m_pressed_value = e.value;
			//setPenConfig(&paint, e.value);
		} 
		
		if (e.type == EV_ABS && e.value !=0) {
			/*if (e.code == 0x0035) {
				//ALOGD("cc x=%d \n",e.value);
				x = e.value*mXScale;
			} else if (e.code == 0x0036) {
				//ALOGD("cc y=%d \n",e.value);
				y = e.value*mYScale;
			} else if(e.code == ABS_X) {//pen
				x = e.value*XScale;
				//ALOGD("---pen x =%d\n",x);
			} else if(e.code == ABS_Y) {
				y = e.value*YScale;
				//ALOGD("---pen y =%d\n",y);
			}*/
			if(e.code == ABS_X) {//pen
				x = e.value*XScale;
				//ALOGD("---pen x =%d\n",x);
			} else if(e.code == ABS_Y) {
				y = e.value*YScale;
				//ALOGD("---pen y =%d\n",y);
			}
		}

		/*if(e.type == EV_KEY && e.code ==BTN_TOOL_PEN) {//is a pen			
			isPen = true;
			penMode = 0;
			ALOGD("---isPen:%d -----\n ", isPen);
		} else if (e.code == 0x39) {//is touch screen
			isPen = false;
			penMode = 1;
			ALOGD("---isPen:%d -----\n", isPen);
		}*/
		
		/*
		*  isTouched: false
		*  It means we haven't draw the canvas any more...
		*/
		//if ((e.type == EV_ABS && (e.code == 0x39) && (e.value == 0xffffffff || e.value == 0))||((e.type == EV_KEY)&&(e.code == BTN_TOOL_PEN)&&(e.value == 0))) {
		if (e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 0) {	
			isPen = false;
			ALOGD("---x:%d,y:%d,,win_x1:%d,win_x2:%d,win_y1:%d,win_y2:%d---", x, y, win_x2, win_y1, win_y2);
			if(x > 154 && x < 1776) {	
			lastX = 0;
			lastY = 0;
			//sendEMREventToJava(x, y, m_pressed_value, penMode, 1);
			/*if(noteVector.size() > 0) {
				vector<vector<PointStruct>>::iterator itPath;
				vector<PointStruct>::iterator itPoint;
				ALOGD("----a1--noteVector.size:%d \n", noteVector.size());
				for (itPath = noteVector.begin(); itPath != noteVector.end(); itPath++)
					{
						for (itPoint = pathVector.begin(); itPoint != pathVector.end(); itPoint++)
							{
								ALOGD("----a1--x:%d,y:%d,m_pressed_value:%d,penMode:%d,action:%d \n", itPoint->x, itPoint->y, itPoint->m_pressed_value, itPoint->penMode, itPoint->action);
							}
					}
			}*/
			noteVector.push_back(pathVector);
			/*vector<vector<PointStruct>>::iterator itPath2;
			vector<PointStruct>::iterator itPoint2;
			vector<PointStruct> vec_tmp;
			for (itPath2 = noteVector.begin(); itPath2 != noteVector.end(); itPath2++)
			{	
				vec_tmp = *itPath2;
				for (itPoint2 = vec_tmp.begin(); itPoint2 != vec_tmp.end(); itPoint2++)
				{
					//ALOGD("----a2--x:%d,y:%d,m_pressed_value:%d,penMode:%d,action:%d \n", itPoint2->x, itPoint2->y, itPoint2->m_pressed_value, itPoint2->penMode, itPoint2->action);
				}
			}*/
    		ALOGD("--a--noteVector.size:%d \n",noteVector.size());
			ALOGD("--a-up---");
				}else {
				ALOGD("--a-outside---");
				return;
				}
		}

		/*
		*  isTouched: true
		*  It means we are writting with the canvas.
		*
		*  this is for rk3288 office-pad
		*/
		//if ((e.type == EV_ABS && (e.code == 0x39) && (e.value == 1))||((e.type == EV_KEY)&&(e.code == BTN_TOOL_PEN)&&(e.value == 1))) {
		if (e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 1) {
			isPen = true;
			/*if(e.code ==BTN_TOOL_PEN) {//is a pen
				isPen = true;
				penMode = 0;
			} else if (e.code == 0x39) {//is touch screen
				isPen = false;
				penMode = 1;
			}*/
			//update the paint's color and size from /data/draw_config
			GetPenConfig(&paint);
			pathVector.clear();
			PointStruct point;
			point.x = x;
			point.y = y;
			point.m_pressed_value = m_pressed_value;
			point.penColor = pen_color;
			point.penWidth = pen_width;
			point.action = 0;
			pathVector.push_back(point);
			ALOGD("--a--pathVector.size:%d \n",pathVector.size());
			//sendEMREventToJava(x, y, m_pressed_value, penMode, 0);
			ALOGD("--a-down,ispen =%d---\n",isPen);
		}

		/*if(e.type == EV_KEY && e.code == BTN_TOUCH )
		 {//pen touch
		 	ALOGD("---e.value=%d\n",e.value);
			if(e.value == 1)
			isPressed = true;
			else
			isPressed = false;
		}*/

		

		if (e.type == EV_SYN) {
			//ALOGD("---lastX=%d--lastY=%d\n", lastX, lastY);
			ALOGD("---isPressed:%d,isPen:%d -----\n", isPressed, isPen);
			if(isPen) {//touchscreen or pen and touch
				if (x>=win_x1 && x<=win_x2 && y>=win_y1 && y<=win_y2) {
					//ALOGD("---start drawline -lastX=%d--lastY=%d--x=%d--y=%d \n",lastX,lastY,x,y);
					ALOGD("---m_pressed_value=%d \n", m_pressed_value);
					if (m_pressed_value <= 2000) {
						pen_width = 1;
					} else if (m_pressed_value > 2000 && m_pressed_value <= 3200) {
						pen_width = ceil((m_pressed_value - 2000) / 300) + 1;
					} else if (m_pressed_value > 3200) {
						pen_width = ceil((m_pressed_value - 3200) / 200) + 5;
					}
					ALOGD("---penWidth=%d \n", pen_width);
					GetPenConfig(&paint);
					if(lastX != 0 || lastY != 0){
						canvas->drawLine(lastX, lastY, x, y, paint);
						//ALOGD("---start drawline -lastX=%d--lastY=%d--x=%d--y=%d \n",lastX,lastY,x,y);
						commitWorker.set_win_info(x, y);
						//sendEMREventToJava(x, y, m_pressed_value, penMode, 2);
						PointStruct point;
						point.x = x;
						point.y = y;
						point.m_pressed_value = m_pressed_value;
						point.penColor = pen_color;
						point.penWidth = pen_width;
						point.action = 2;
						pathVector.push_back(point);
						ALOGD("--a--m_pressed_value:%d \n", m_pressed_value);
					} 
					lastX = x;
					lastY = y;
					commitWorker.Signal();
				} else {
					lastX = 0;
					lastY = 0;
				}
			}
		}
	}
    //path.close();
    return;
}

void PaintWorker::SetPenColor(int color)
{
	pen_color = color;
}

void PaintWorker::SetPenWidth(int width)
{
	pen_width = width;
}

void PaintWorker::SetPenMode(int mode)
{
	pen_mode = mode;
}


void PaintWorker::SetPenOrEraserFromMode()
{
	if(pen_mode == PEN_MODE) {
		SetPenColor(mPenColor);
		SetPenWidth(mPenColor);
	} else if(pen_mode == ERASER_MODE) {
		SetPenColor(mEraserColor);
		SetPenWidth(mEraserWidth);
	}
}

void PaintWorker::SetDrawingStatus(int isDrawing)
{
	this->isDrawing = isDrawing;
}


bool PaintWorker::GetPenConfig(SkPaint *paint)
{
	if (pen_color != pen_old_color) {
		if (pen_color == PEN_ALPHA_COLOR)
			paint->setARGB(255, 255, 255, 255);
		if (pen_color == PEN_BLACK_COLOR)
			paint->setARGB(255, 0, 0, 0);
		if (pen_color == PEN_WHITE_COLOR)
			paint->setARGB(255, 30, 30, 30);
		pen_old_color = pen_color;
	}
	if (pen_width != pen_old_width) {
		paint->setStrokeWidth(pen_width);
		pen_old_width = pen_width;
		commitWorker.setPenWidth(pen_width);
	}
	return true;
}

void PaintWorker::clearScreen(bool isClearScrenn)
{
	commitWorker.clearScreen(isClearScrenn);
	if(isClearScrenn)
		noteVector.clear();
}

void PaintWorker::dump(void)
{
	commitWorker.dump();
}

