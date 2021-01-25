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

#ifndef ANDROID_PAINT_WORKER_H_
#define ANDROID_PAINT_WORKER_H_

#include "worker.h"
#include <SkTypeface.h>
//#include <SkTemplates.h>

#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkRegion.h>
#include <SkPath.h>

#include "../../../../external/skia/include/private/SkTemplates.h"

#include <system/graphics.h>

#include <commitworker.h>

using namespace android;


class PaintWorker : public Worker {
public:
    PaintWorker();
    ~PaintWorker() override;
    int Init(int left, int top, int right, int bottom);
    bool GetPenConfig(SkPaint  *paint);
    void *Get_path_buffer();
    void ExitPaintWorker();
	void ReDrawPoint(int x, int y, int action);
	void ShowDrawPoint(int left, int top, int right, int bottom);
    void SetPenColor(int color);
    void SetPenWidth(int width);
	void SetPenMode(int mode);
    void clearScreen(bool isClearScrenn);
	void SetDrawingStatus(int isDrawing);
    void dump(void);
	int Undo(int left, int top, int right, int bottom);
	int ReDraw(int left, int top, int right, int bottom);
	void ShowReDraw(int x, int y);
	void SetPenOrEraserFromMode();
protected:
    void Routine() override;

private:

    void onFirstRef();
    //SkCanvas *canvas;



    void *rgba_buffer;
    //CommitWorker çº¿ç¨‹ç±»
    CommitWorker commitWorker;
    int pen_color;
    int pen_old_color;
    int pen_width;
    int pen_old_width;
	int isDrawing;
	int pen_mode;
};

#define PEN_ALPHA_COLOR 0 //Í¸Ã÷É«
#define PEN_BLACK_COLOR 1 //ºÚÉ«
#define PEN_WHITE_COLOR 2 //°×É«
#define PEN_MODE 		3 //画笔模式
#define ERASER_MODE 	4 //橡皮擦模式

#endif

