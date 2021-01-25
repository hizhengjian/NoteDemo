/*
 * Copyright 2009 Cedric Priscal
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <jni.h>

#include <pthread.h>
#include <linux/input.h>

#include <cutils/properties.h>
#include "paintworker.h"

#include "fields.h"

#include "android/log.h"

static const char *TAG = "NDK_WeNote";

#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

#define NUM_STEPS 10 //越大，曲线越密，越逼近

typedef struct {
   float x, y;
} Point;


struct Fields fields;

static JavaVM *g_wenote_jvm = NULL;
static jobject g_wenote_obj = NULL;
//static JNIEnv *m_flash_env;
//static jclass m_class_wenoteview;
static jobject m_obj_class_loader = NULL;
static jmethodID m_method_find_class = NULL;
jmethodID m_method_receiveemrevent;
static jmethodID m_method_forceupdatejava;

static int m_pen_mode = -1;

Point point[4];
static PaintWorker *paintWorker;
static CommitWorker *commitWorker;

int on_down(float ex, float ey);
int on_move(float ex, float ey);
int on_up();
int on_update();
void initPainter(int left, int top, int right, int bottom, char *path);

void deinitField(JNIEnv* env);
int initFields(JNIEnv* env);

void setJavaUIMode(int isJava);

using namespace std;
class CvPoint
{
public:
	float x;
	float y;
	CvPoint()
	{
		x = 0.0;
		y = 0.0;
	}
	CvPoint(float a,float b)
	{
		x = a;
		y = b;
	}

};

extern "C" {
JNIEXPORT jbyteArray JNICALL Java_com_intelgine_handwrite_HandWrite_getPixels
(JNIEnv *env, jobject obj);
};

char* jstringTostring(JNIEnv* env, jstring jstr)
{
	char* rtn = NULL;
	jclass clsstring = env->FindClass("java/lang/String");
	jstring strencode = env->NewStringUTF("utf-8");
	jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
	jsize alen = env->GetArrayLength(barr);
	jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
	if (alen > 0)
	{
		rtn = (char*)malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	env->ReleaseByteArrayElements(barr, ba, 0);
	return rtn;
}

/*
 * Class:     native_init
 * Method:    init
 * Signature: ()V
 */
//JNIEXPORT jint JNICALL native_init(JNIEnv * env, jobject thiz, jobject classview)
JNIEXPORT jint JNICALL native_init(JNIEnv * env, jobject thiz, jobject jrect, jstring file_path)
{
	LOGD("Flash test : +++++++++++++ native_init()");
	//initFields(env);
	//m_class_wenoteview = env->GetObjectClass(classview);
	//m_method_receiveemrevent = env->GetStaticMethodID(m_class_wenoteview, "receiveEMREvent", "(III)V");
	//m_flash_env = env;

	jclass clazz;
	jfieldID jfid;
	clazz = env->GetObjectClass(jrect);
	if (0 == clazz) {
		LOGE("Flash test : +++++ native_init() GetObjectClass returned 0\n");
		return JNI_ERR;
	}

	int left = 0, top = 0, right = 0, bottom = 0; 

	jfid = (env)->GetFieldID(clazz, "left", "I");
	left = (int)env->GetIntField(jrect, jfid);

	jfid = (env)->GetFieldID(clazz, "top", "I");
	top = (int)env->GetIntField(jrect, jfid);

	jfid = (env)->GetFieldID(clazz, "right", "I");
	right = (int)env->GetIntField(jrect, jfid);

	jfid = (env)->GetFieldID(clazz, "bottom", "I");
	bottom = (int)env->GetIntField(jrect, jfid);

	LOGD("Flash test : +++++++++++++ native_init() read Rect(%d, %d, %d, %d)\n", left, top, right, bottom);

	//int height = (int)jheight;
	char *back_path;
	back_path = jstringTostring(env, file_path);
	initPainter(left, top, right, bottom, back_path);

	//env->GetJavaVM(&g_wenote_jvm);
	//不能直接赋值(g_obj = obj)
	g_wenote_obj = env->NewGlobalRef(thiz);
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_touch_down(JNIEnv * env, jobject thiz, jfloat x, jfloat y)
{
	LOGD("Flash test : +++++++++++++ native_touch_down()");
	return JNI_OK;
}


/*
 * Class:     native_add_point
 * Method:    add new point to jni
 * Signature: ()V
 */
JNIEXPORT jint JNICALL native_add_point(JNIEnv * env, jobject thiz, jfloat x, jfloat y)
{
	LOGD("Flash test : +++++++++++++ add new point (%f, %f)", x, y);

	return JNI_OK;
}


JNIEXPORT jint JNICALL native_update(JNIEnv * env, jobject thiz)
{
	LOGD("Flash test : +++++++++++++ native_update()");
	return JNI_OK;
}


JNIEXPORT jint JNICALL native_touch_up(JNIEnv * env, jobject thiz)
{
	LOGD("Flash test : +++++++++++++ native_touch_up()");
	return JNI_OK;
}


JNIEXPORT jint JNICALL native_clear(JNIEnv * env, jobject thiz)
{
	LOGD("Flash test : +++++++++++++ native_clear()");
	paintWorker->clearScreen(true);
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_save(JNIEnv * env, jobject thiz, jstring file_path)
{
	LOGD("Flash test : +++++++++++++ native_save()");
    /*
	char *save_path;
	save_path = jstringTostring(env, file_path);
	if (paintWorker->saveNoteToPNG(save_path) == JNI_TRUE) {
		return JNI_OK;
	} else {
		return JNI_ERR;
	}*/
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_exit(JNIEnv * env, jobject thiz)
{
	LOGD("Flash test : +++++++++++++ native_exit()");
	if (paintWorker == NULL) {
		return JNI_OK;
	}

	//property_set("debug.property.enable","0");
	//property_set("debug.mode","0");
	setJavaUIMode(1);
	//usleep(1000*100);

	paintWorker->ExitPaintWorker();
    delete paintWorker;
    paintWorker = NULL;
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_set_display_mode(JNIEnv * env, jobject thiz, jint mode)
{
	LOGD("Flash test : +++++++++++++ native_set_display_mode()");
	//setJavaUIMode((int)mode);
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_enable(JNIEnv * env, jobject thiz, jint status)
{
	LOGD("Flash test : +++++++++++++ native_enable():%d", status);
	if(status == 0) {
		paintWorker->SetRoutineEnable(false);
	} else {
		paintWorker->ExitPaintWorker();
		paintWorker->SetRoutineEnable(true);
	}
    delete paintWorker;
    paintWorker = NULL;;
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_draw_point(JNIEnv * env, jobject thiz, jint x, jint y, jint action)
{
	LOGD("Flash test : +++++++++++++ native_draw_point() x:%d, y:%d", x, y);
	paintWorker->ReDrawPoint(x, y, action);
    //delete paintWorker;
    //paintWorker = NULL;;
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_show_point(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
	LOGD("Flash test : +++++++++++++ native_show_point()");
	paintWorker->ShowDrawPoint(left, top, right, bottom);
    //delete paintWorker;
    //paintWorker = NULL;;
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_dump(JNIEnv * env, jobject thiz)
{
	LOGD("Flash test : +++++++++++++ native_dump()");
	commitWorker->dump();
    //delete paintWorker;
    //paintWorker = NULL;;
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_undo(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
	LOGD("Flash test : +++++++++++++ native_undo()");
	int status = paintWorker->Undo(left, top, right, bottom);
	//LOGD("Flash test : +++++++++++++ status:%d", status);
	return status;
}

JNIEXPORT jint JNICALL native_redraw(JNIEnv * env, jobject thiz, jint left, jint top, jint right, jint bottom)
{
	LOGD("Flash test : +++++++++++++ native_redraw()");
	int status = paintWorker->ReDraw(left, top, right, bottom);
	return status;
}

JNIEXPORT jint JNICALL native_eraser(JNIEnv * env, jobject thiz, int color, int width)
{
	LOGD("Flash test : +++++++++++++ native_eraser()");
	paintWorker->SetPenColor(color);
	paintWorker->SetPenWidth(width);
	return JNI_OK;
}

JNIEXPORT jint JNICALL native_set_is_drawing(JNIEnv * env, jobject thiz, int isDrawing)
{
	LOGD("Flash test : +++++++++++++ native_set_is_drawing()");
	paintWorker->SetDrawingStatus(isDrawing);
	return JNI_OK;
}



JNIEXPORT jbyteArray JNICALL Java_com_intelgine_handwrite_HandWrite_getPixels(JNIEnv * env, jobject thiz)
{
	LOGD("Flash test : +++++++++++++ getPixels");
	jbyte by[1200*825];
	int i=0;

	jbyteArray jarray = env->NewByteArray(1200*825);
	env->SetByteArrayRegion(jarray, 0, 1200*825, by);

	//env->ReleaseIntArrayElements(buf, mJavaBuf, 0);
	return jarray;
}

static JNINativeMethod gMethods[] = {
	{ "native_init", 	"(Landroid/graphics/Rect;Ljava/lang/String;)I",	(void*) native_init },
	//{ "native_init", 	"(Lcom/wetao/note/WeNoteView;)I",	(void*) native_init },
	{ "native_enable", 	"(I)I",	(void*) native_enable },
	{ "native_exit", 	"()I",	(void*) native_exit },
	{ "native_touch_down", 	"(FF)I",	(void*) native_touch_down },
	{ "native_touch_up", 	"()I",	(void*) native_touch_up },
	{ "native_add_point", "(FF)I",  (void*) native_add_point },
	{ "native_clear",       "()I",  (void*) native_clear },
	{ "native_save",       "(Ljava/lang/String;)I",  (void*) native_save },
	{ "native_update",      "()I",  (void*) native_update },
	{ "native_set_display_mode",      "(I)I",  (void*) native_set_display_mode },
	{ "native_draw_point",      "(III)I",  (void*) native_draw_point },
	{ "native_show_point",      "(IIII)I",  (void*) native_show_point },
	{ "native_dump",      "()I",  (void*) native_dump },
	{ "native_undo",      "(IIII)I",  (void*) native_undo },
	{ "native_redraw",      "(IIII)I",  (void*) native_redraw },
	{ "native_eraser",      "(II)I",  (void*) native_eraser },
	{ "native_set_is_drawing",      "(I)I",  (void*) native_set_is_drawing }
};

/*
 * 为某一个类注册本地方法
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
		JNINativeMethod* gMethods, int numMethods) {
	jclass clazz;
	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOGD("---------clazz is null");
		return JNI_FALSE;
	}
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		LOGD("---------RegisterNatives < 0");
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

/*
 * 为所有类注册本地方法
 */
static int registerNatives(JNIEnv* env) {
	const char* kClassName = "com/rockchip/notedemo/NoteJNI"; //指定要注册的类com.boncare.chair.serialport
	return registerNativeMethods(env, kClassName, gMethods,
			sizeof(gMethods) / sizeof(gMethods[0]));
}

void forceUpdateJava() {
	JNIEnv *env = NULL;
	int isAttacked = 0;
	jclass clazz = NULL;
	jstring class_name = NULL;
	jobject cls_obj = NULL;
	int result;
	int status = g_wenote_jvm->GetEnv((void**) &env, JNI_VERSION_1_4);
	if (status < 0) {
		isAttacked = 1;
		JavaVMAttachArgs args = {JNI_VERSION_1_4, NULL, NULL};
		result = g_wenote_jvm->AttachCurrentThread(&env, (void*) &args);
		if (result != JNI_OK) {
			LOGE("thread attach failed: %#x", result);
			goto error;
		}
	}

	class_name = env->NewStringUTF("com/rockchip/notedemo/NoteJNI");
	cls_obj = env->CallObjectMethod(m_obj_class_loader, m_method_find_class, class_name);
	clazz = env->GetObjectClass(cls_obj);

	if (clazz != NULL) {
		env->CallStaticVoidMethod( clazz, m_method_forceupdatejava);
	}
	env->DeleteLocalRef(clazz);
	env->DeleteLocalRef(class_name);
	env->DeleteLocalRef(cls_obj);

error:    
     //Detach主线程
     if(g_wenote_jvm->DetachCurrentThread() != JNI_OK)
     {
         LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
     }
}

void sendEMREventToJava(int x, int y, int press, int mode, int action) { //receiveEMREvent
	//LOGD("Flash test : +++++++ sendEMREventToJava action = %d", action);

	JNIEnv *env = NULL;
	int isAttacked = 0;
	jclass clazz = NULL;
	jstring class_name = NULL;
	jobject cls_obj = NULL;
	int result;
	
	int status = g_wenote_jvm->GetEnv((void**) &env, JNI_VERSION_1_6);
	if (status < 0) {
		isAttacked = 1;
		JavaVMAttachArgs args = {JNI_VERSION_1_6, NULL, NULL};

		result = g_wenote_jvm->AttachCurrentThread(&env, (void*) &args);
		if (result != JNI_OK) {
			LOGE("thread attach failed: %#x", result);
			goto error;
		}
	}
	
	
	class_name = env->NewStringUTF("com/rockchip/notedemo/NoteJNI");
	LOGD("Flash test : ++++++ class_name = %s", jstringTostring(env, class_name));
	cls_obj = env->CallObjectMethod(m_obj_class_loader, m_method_find_class, class_name);

	clazz = env->GetObjectClass(cls_obj);


	if (clazz != NULL) {

		//m_method_receiveemrevent = env->GetStaticMethodID(clazz, "receiveEMREvent","(IIIII)V");
		env->CallStaticVoidMethod(clazz, m_method_receiveemrevent, x, y, press, mode, action);
	}
	env->DeleteLocalRef(clazz);
	env->DeleteLocalRef(class_name);
	env->DeleteLocalRef(cls_obj);

error:    
     //Detach主线程
     if(g_wenote_jvm->DetachCurrentThread() != JNI_OK)
     {
         LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
     }
}

void setJavaUIMode(int isJava) {
	LOGD("Flash test : +++++++ setJavaUIMode new mode isJava = %d, old mode m_pen_mode = %d", isJava, m_pen_mode);
	if (m_pen_mode == isJava) return;

	if (isJava == 1) {
		//usleep(1000*1500);
		property_set("debug.property.enable","0");
		property_set("debug.mode","0");
		//usleep(1000*200);
		forceUpdateJava();
	} else {
		property_set("debug.property.enable","1");
		property_set("debug.mode","8");
		//usleep(1000*100);
	}
	m_pen_mode = isJava;
}

void deinitField(JNIEnv* env)
{
	if(fields.IllegalStateException.clazz)
    	env->DeleteGlobalRef(fields.IllegalStateException.clazz);
	if(fields.String.clazz)
   		env->DeleteGlobalRef(fields.String.clazz);
	if(fields.WeNoteView.clazz)
		env->DeleteGlobalRef(fields.WeNoteView.clazz);
	if(fields.app.clazz)
		env->DeleteGlobalRef(fields.app.clazz);
	if(fields.app.inst)
		env->DeleteGlobalRef(fields.app.inst);
}



void initPainter(int left, int top, int right, int bottom, char *path) {
	//LOGD("Flash test : +++++++++ initPainter() height = %d\n", height);
	//初始化 PaintWorker 线程类
	paintWorker = new PaintWorker();
	paintWorker->Init(left, top, right, bottom);
	//paintWorker.Exit();
	fprintf(stderr, "Drawpath end!---------------------------- \n");

	setJavaUIMode(0);
	//forceUpdateJava();
}

/*
 * System.loadLibrary("lib")时调用
 * 如果成功返回JNI版本, 失败返回-1
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	LOGD("Flash test : ***** JNI_OnLoad.");
	JNIEnv* env = NULL;
	jint result = -1;
	g_wenote_jvm = vm;

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		return JNI_ERR;
	}
	assert(env != NULL);

	jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
	jclass adapterClass = env->FindClass("com/rockchip/notedemo/NoteJNI");

	if(adapterClass) {
		jmethodID getClassLoader = env->GetStaticMethodID(adapterClass,"getClassLoader","()Ljava/lang/ClassLoader;");
		m_method_receiveemrevent = env->GetStaticMethodID(adapterClass, "receiveEMREvent","(IIIII)V");
		m_method_forceupdatejava = env->GetStaticMethodID(adapterClass, "forceUpdateJava","()V");

		jobject obj = env->CallStaticObjectMethod(adapterClass,getClassLoader);
	
		env->CallStaticVoidMethod(adapterClass,m_method_receiveemrevent, 0, 0, 0, 0, 0);
		env->CallStaticVoidMethod(adapterClass,m_method_forceupdatejava);
	
		m_obj_class_loader = env->NewGlobalRef( obj);
		m_method_find_class = env->GetMethodID(classLoaderClass, "loadClass","(Ljava/lang/String;)Ljava/lang/Class;");
	
		env->DeleteLocalRef(classLoaderClass);
		env->DeleteLocalRef(adapterClass);
		env->DeleteLocalRef(obj);
	}

	if (!registerNatives(env)) { //注册
		return JNI_ERR;
	}

	//initFields(env);
	//成功
	result = JNI_VERSION_1_6;

	return result;
}

void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return;

	LOGD("Flash test : ***** JNI_OnUnload.");

	//deinitField(env);
}

