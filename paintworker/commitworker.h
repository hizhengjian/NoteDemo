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

#ifndef ANDROID_COMMIT_WORKER_H_
#define ANDROID_COMMIT_WORKER_H_

#include "worker.h"
#include <system/graphics.h>

using namespace android;

#if 1
#define EINK_FB_SIZE		0x400000 /* 4M */

/*
 * ebc system ioctl command
 */
#define EBC_GET_BUFFER			(0x7000)
#define EBC_SEND_BUFFER			(0x7001)
#define EBC_GET_BUFFER_INFO		(0x7002)
#define EBC_SET_FULL_MODE_NUM	(0x7003)
#define EBC_ENABLE_OVERLAY		(0x7004)
#define EBC_DISABLE_OVERLAY		(0x7005)

/*
 * IMPORTANT: Those values is corresponding to android hardware program,
 * so *FORBID* to changes bellow values, unless you know what you're doing.
 * And if you want to add new refresh modes, please appended to the tail.
 */
enum panel_refresh_mode {
	EPD_AUTO			= 0,
	EPD_OVERLAY		= 1,
	EPD_FULL_GC16		= 2,
	EPD_FULL_GL16		= 3,
	EPD_FULL_GLR16		= 4,
	EPD_FULL_GLD16		= 5,
	EPD_FULL_GCC16		= 6,
	EPD_PART_GC16		= 7,
	EPD_PART_GL16		= 8,
	EPD_PART_GLR16		= 9,
	EPD_PART_GLD16		= 10,
	EPD_PART_GCC16		= 11,
	EPD_A2				= 12,
	EPD_A2_DITHER		= 13,
	EPD_DU				= 14,
	EPD_RESET			= 15,
	EPD_SUSPEND		= 16,
	EPD_RESUME			= 17,
	EPD_POWER_OFF		= 18,
	EPD_FULL_DIRECT	= 19,
	EPD_PART_DIRECT	= 20,
	EPD_A2_DIRECT		= 21,
	EPD_DU_DIRECT		= 22,
	EPD_AUTO_DIRECT	= 23,
	EPD_OVERLAY_DIRECT	= 24,
	EPD_PART_EINK		= 25,
	EPD_FULL_EINK		= 26,
};

/*
 * IMPORTANT: android hardware use struct, so *FORBID* to changes this, unless you know what you're doing.
 */
struct ebc_buf_info_t {
	int offset;
	int epd_mode;
	int height;
	int width;
	int panel_color;
	int win_x1;
	int win_y1;
	int win_x2;
	int win_y2;
	int width_mm;
	int height_mm;
};

#else
#define GET_EBC_BUFFER 0x7000
#define SET_EBC_SEND_BUFFER 0x7001
#define GET_EBC_BUFFER_INFO 0x7003
#define ENABLE_EBC_OVERLAY (0x7007)
#define DISABLE_EBC_OVERLAY (0x7008)

#define EPD_NULL            (-1)
#define EPD_AUTO            (0)
#define EPD_FULL            (1)
#define EPD_A2              (2)
#define EPD_PART            (3)
#define EPD_FULL_DITHER     (4)
#define EPD_RESET           (5)
#define EPD_BLACK_WHITE     (6)
#define EPD_BG            (7)
#define EPD_BLOCK           (8)
#define EPD_FULL_WIN        (9)
#define EPD_OED_PART		(10)
#define EPD_DIRECT_PART     (11)
#define EPD_DIRECT_A2       (12)
#define EPD_STANDBY			(13)
#define EPD_POWEROFF        (14)
#define EPD_NOPOWER        (15)
#define EPD_AUTO_BG        (16)

/*android use struct*/
struct ebc_buf_info_t{
  int offset;
  int epd_mode;
  int height;
  int width;
  int vir_height;
  int vir_width;
  int fb_width;
  int fb_height;
  int color_panel;
  int left;
  int top;
  int right;
  int bottom;
  int rotate;
  int width_mm;
  int height_mm;
}__packed;
#endif

struct win_coordinate{
  int left;
  int top;
  int right;
  int bottom;
};


class CommitWorker : public Worker {
public:
    CommitWorker();
    ~CommitWorker() override;
    int Init();
    void set_path_buffer(void *path_buffer);
    void set_win_info(int x, int y);
    void set_point_info(int x, int y);
    void set_first_win_info();
    void init_draw_win_info(int x1, int y1, int x2, int y2);
    void setPenWidth(int width);
    void clearScreen(bool isClearScrenn);
    void dump(void);
protected:
    void Routine() override;

private:

    void onFirstRef();
    

    int ebc_fd = -1;
    void *ebc_buffer_base = NULL;
    struct ebc_buf_info_t ebc_buf_info;
    void *rgba_buffer;
    int *gray16_buffer;
    win_coordinate win_info;
    int last_x = 0;
    int last_y = 0;
    bool update_enable = false;
    int pen_width;
};

//default draw 1byte gray image, if you want to use rgba image open following define
//#define SKIA_DRAW_RGB_IMAGE

#endif


