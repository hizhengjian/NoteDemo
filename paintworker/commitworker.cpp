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

#include <commitworker.h>


#include <unistd.h>
#include <sys/mman.h>

#include <cutils/properties.h>

//open header
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//map header
#include <map>

#include <time.h>

#include <ui/Region.h>

#include <utils/Log.h>
#include <utils/Timers.h>


using namespace android;

static pthread_mutex_t win_info_lock;

void Luma8bit_to_4bit_fix(short int  *src,  char *dst, int w)
{
    int i;
    int g0, g1;
    int src_data;

    for(i=0; i<w; i+=2)
    {
        src_data =  *src++;
        g0 = src_data&0x0f;

        g1 = ((src_data&0x0f00)>>4);

        *dst++ = g1|g0;
    }
}

void gray256_to_gray2_fix(uint8_t *dst, uint8_t *src, int panel_h, int panel_w,
int vir_width, Region region)
{
	size_t count = 0;
	const Rect* rects = region.getArray(&count);
	for (int i = 0;i < (int)count;i++) {		

		int w = rects[i].right - rects[i].left;
		int offset = rects[i].top * panel_w + rects[i].left;
		int offset_dst = rects[i].top * vir_width + rects[i].left;
		if (offset_dst % 2) {
			offset_dst += (2 - offset_dst % 2);
		}
		if (offset % 2) {
			offset += (2 - offset % 2);
		}
		if ((offset_dst + w) % 2) {
			w -=  (offset_dst + w) % 2;
		}
		for (int h = rects[i].top;h <= rects[i].bottom && h < panel_h;h++) {
			//LOGE("jeffy Luma8bit_to_4bit_row_2, w:%d, offset:%d, offset_dst:%d", w, offset, offset_dst);
			Luma8bit_to_4bit_fix((short int*)(src + offset), (char*)(dst + (offset_dst >> 1)), w);
			offset += panel_w;
			offset_dst += vir_width;
		}
	}
}

CommitWorker::CommitWorker()
    : Worker("CommitWorker", HAL_PRIORITY_URGENT_DISPLAY) {
    fprintf(stderr, "new CommitWorker\n");

}

CommitWorker::~CommitWorker(){
    fprintf(stderr, "~CommitWorker() \n");
	//apk完全退出时，可能需要销毁
    //pthread_mutex_destroy(&win_info_lock);
    if(ioctl(ebc_fd, EBC_DISABLE_OVERLAY, NULL) != 0) {
        ALOGE("DISABLE_EBC_OVERLAY failed\n");
    }
    if(ebc_fd > 0){
        close(ebc_fd);
    }
    if(ebc_buffer_base != NULL){
        munmap(ebc_buffer_base, EINK_FB_SIZE * 4);
        ebc_buffer_base = NULL;
    }
    if(gray16_buffer != NULL){
        free(gray16_buffer);
        gray16_buffer = NULL;
    }
    if(rgba_buffer != NULL){
        rgba_buffer = NULL;
    }
}

int CommitWorker::Init(){
  ebc_fd = open("/dev/ebc", O_RDWR,0);
  if (ebc_fd < 0){
      ALOGE("open /dev/ebc failed\n");
  }

  if(ioctl(ebc_fd, EBC_GET_BUFFER_INFO,&ebc_buf_info)!=0){
      ALOGE("GET_EBC_BUFFER failed\n");
  }

  ebc_buffer_base = mmap(0, EINK_FB_SIZE * 4, PROT_READ|PROT_WRITE, MAP_SHARED, ebc_fd, 0);
  if (ebc_buffer_base == MAP_FAILED) {
      ALOGE("Error mapping the ebc buffer (%s)\n", strerror(errno));
  }

  gray16_buffer = (int *)malloc(ebc_buf_info.width * ebc_buf_info.height >> 1);
  memset(gray16_buffer, 0xff, ebc_buf_info.width * ebc_buf_info.height >> 1);
  if(ioctl(ebc_fd, EBC_ENABLE_OVERLAY, NULL) != 0) {
      ALOGE("ENABLE_EBC_OVERLAY failed\n");
  }

  pthread_mutex_init(&win_info_lock, NULL);

  update_enable = false;
  return InitWorker();
}

void CommitWorker::set_path_buffer(void *path_buffer){
  rgba_buffer = path_buffer;
}

void CommitWorker::setPenWidth(int width){
  pen_width = width + 10;
}

void CommitWorker::set_point_info(int x, int y){
  pthread_mutex_lock(&win_info_lock);
  last_x = x;
  last_y = y;
  update_enable = true;
  win_info.left = x;
  win_info.right = x;
  win_info.top = y;
  win_info.bottom = y;
  pthread_mutex_unlock(&win_info_lock);
}

void CommitWorker::set_win_info(int x, int y){
  pthread_mutex_lock(&win_info_lock);
  last_x = x;
  last_y = y;
  update_enable = true;
  if(win_info.left > x)
    win_info.left = x;
  if(win_info.right < x)
    win_info.right = x;
  if(win_info.top > y)
    win_info.top = y;
  if(win_info.bottom < y)
    win_info.bottom = y;
  pthread_mutex_unlock(&win_info_lock);
}

void CommitWorker::set_first_win_info(){
    win_info.left = last_x;
    win_info.right = last_x;
    win_info.top = last_y;
    win_info.bottom = last_y;
    update_enable = false;
}

void CommitWorker::init_draw_win_info(int x1, int y1, int x2, int y2)
{
  ALOGE("Flash test : +++++++++++++ native_init() read Rect(%d, %d, %d, %d)\n", x1, y1, x2, y2);
  pthread_mutex_lock(&win_info_lock);
  win_info.left = x1;
  win_info.right = x2;
  win_info.top = y1;
  win_info.bottom = y2;
  pthread_mutex_unlock(&win_info_lock);
}

void CommitWorker::Routine(){
	if(!exit_){
  //fprintf(stderr, "CommitWorker Routine\n");
  int ret = Lock();
  if (ret) {
    ALOGE("Failed to lock worker, %d", ret);
    return;
  }

  int wait_ret = 0;
  if (rgba_buffer != NULL && ebc_buffer_base != NULL) {
      if(update_enable == false)
        wait_ret = WaitForSignalOrExitLocked();

      ret = Unlock();
      if (ret) {
        ALOGE("Failed to lock worker, %d", ret);
        return;
      }

      struct ebc_buf_info_t buf_info;

      if(ioctl(ebc_fd, EBC_GET_BUFFER,&buf_info)!=0)
      {
         ALOGE("GET_EBC_BUFFER failed\n");
         return;
      }

      pthread_mutex_lock(&win_info_lock);
      buf_info.win_x1 = win_info.left - pen_width >= 0 ? win_info.left - pen_width : 0;
      buf_info.win_y1 = win_info.top - pen_width >= 0 ? win_info.top - pen_width : 0;
      buf_info.win_x2 = win_info.right + pen_width <= ebc_buf_info.width ? win_info.right + pen_width : ebc_buf_info.width;
      buf_info.win_y2 = win_info.bottom + pen_width <= ebc_buf_info.height ? win_info.bottom + pen_width : ebc_buf_info.height;
      set_first_win_info();
      pthread_mutex_unlock(&win_info_lock);
      unsigned long vaddr_real = intptr_t(ebc_buffer_base);

      Rect rect = Rect(buf_info.win_x1, buf_info.win_y1, buf_info.win_x2, buf_info.win_y2);
      Region region = Region(rect);

      buf_info.epd_mode = EPD_OVERLAY;

	//neon_rgb888_to_gray16ARM((uint8_t *)(gray16_buffer), (uint8_t *)rgba_buffer, ebc_buf_info.height, ebc_buf_info.width,ebc_buf_info.width);
	//rgb888_to_gray2_dither((uint8_t *)(gray16_buffer), (uint8_t *)rgba_buffer, ebc_buf_info.height, ebc_buf_info.width,ebc_buf_info.width,region);
#ifdef SKIA_DRAW_RGB_IMAGE
	rgb888_to_gray2_fix((uint8_t *)(gray16_buffer), (uint8_t *)rgba_buffer, ebc_buf_info.height, ebc_buf_info.width,ebc_buf_info.width,region);
#else
	gray256_to_gray2_fix((uint8_t *)(gray16_buffer), (uint8_t *)rgba_buffer, ebc_buf_info.height, ebc_buf_info.width,ebc_buf_info.width,region);
#endif
      //rgb888_to_gray16_dither(gray16_buffer, (uint8_t *)rgba_buffer, ebc_buf_info.height, ebc_buf_info.width,ebc_buf_info.width);
      memcpy((void *)(vaddr_real + buf_info.offset), gray16_buffer,
              buf_info.height * buf_info.width >> 1);

      if(ioctl(ebc_fd, EBC_SEND_BUFFER,&buf_info)!=0)
      {
         fprintf(stderr,"SET_EBC_SEND_BUFFER failed\n");
         return;
      }

      return;
    }else{
    int ret = Unlock();
    if (ret) {
      ALOGE("Failed to lock worker, %d", ret);
      return;
    }
    return;
  }
	} else {
		return;
	}
}


void CommitWorker::clearScreen(bool isClearScrenn)
{
#ifdef SKIA_DRAW_RGB_IMAGE
      memset(rgba_buffer, 0xff, ebc_buf_info.height * ebc_buf_info.width * 4);
#else
      memset(rgba_buffer, 0xff, ebc_buf_info.height * ebc_buf_info.width);
#endif
      memset(gray16_buffer, 0xff, ebc_buf_info.height * ebc_buf_info.width >> 1);
	if(isClearScrenn) {
      struct ebc_buf_info_t buf_info;

      if(ioctl(ebc_fd, EBC_GET_BUFFER,&buf_info)!=0)
      {
         ALOGE("GET_EBC_BUFFER failed\n");
         return;
      }

      unsigned long vaddr_real = intptr_t(ebc_buffer_base);
      buf_info.win_x1 = 0;
      buf_info.win_y1 = 0;
      buf_info.win_y2 = buf_info.height;
      buf_info.win_x2 = buf_info.width;
      buf_info.epd_mode = EPD_OVERLAY;

      memcpy((void *)(vaddr_real + buf_info.offset), gray16_buffer,
              buf_info.height * buf_info.width >> 1);

      if(ioctl(ebc_fd, EBC_SEND_BUFFER,&buf_info)!=0)
      {
         fprintf(stderr,"SET_EBC_SEND_BUFFER failed\n");
         return;
      }
	}

}

void CommitWorker::dump(void)
{
      char value[PROPERTY_VALUE_MAX];
      char data_name[100] ;

      sprintf(data_name,"/data/dump/gray8bit_%d_%d.bin", ebc_buf_info.width, ebc_buf_info.height);
      FILE *file = fopen(data_name, "wb+");
      if (file) {
	  #ifdef SKIA_DRAW_RGB_IMAGE
          fwrite(rgba_buffer, ebc_buf_info.height * ebc_buf_info.width * 4, 1, file);
	  #else
          fwrite(rgba_buffer, ebc_buf_info.height * ebc_buf_info.width, 1, file);
	  #endif
          fclose(file);
      }

      sprintf(data_name,"/data/dump/gray4bit_%d_%d.bin", ebc_buf_info.width, ebc_buf_info.height);
      file = fopen(data_name, "wb+");
      if (file) {
          fwrite(gray16_buffer, ebc_buf_info.height * ebc_buf_info.width >> 1, 1, file);
          fclose(file);
      }
}
