package com.rockchip.notedemo;

import android.graphics.Rect;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.rockchip.notedemo.utils.FlashPoint;

public class NoteJNI {
    private static final String TAG = "NoteJNI";
    public static final int MSG_FLAG_TOUCH	= 10000;
    public static final int MSG_FLAG_UPDATE	= 10001;

    private static Handler mPointHandler = null;

    // JNI
    public native int native_init(Rect rect, String file_path);
    public native int native_enable(int status);
    public native int native_exit();
    public native int native_touch_down(float x, float y);
    public native int native_touch_up();
    public native int native_add_point(float x, float y);
    public native int native_clear();
    public native int native_save(String file_path);
    public native int native_update();
    public native int native_set_display_mode(int mode);
    public native int native_draw_point(int x, int y, int action);
    public native int native_show_point(int left, int top, int right, int bottom);
    public native int native_dump();
    public native int native_undo(int left, int top, int right, int bottom);
    public native int native_redraw(int left, int top, int right, int bottom);
    public native int native_eraser(int mode);
    public native int native_set_is_drawing(int isDrawing);

    public NoteJNI() {
        Log.d(TAG, "NoteJNI");
        //native_init(height);
        //receiveEMREvent(0.0f, 0.0f, 3, 0.0f, 0, 0, -1, true);
        getClassLoader();
        System.loadLibrary("paintworker");
    }

    public int init(Rect rect, String file_path) {
        Log.d(TAG, "Flash test : ++++++++++ init() rect = " + rect);
        int initStatus = native_init(rect, file_path);
        return initStatus;
    }


    public static ClassLoader getClassLoader() {
        Log.d(TAG,"zj add getClassLoader");
        return NoteJNI.class.getClassLoader();
    }

    public static void receiveEMREvent(int x, int y, int press, int mode, int action) {
       /* Log.d(TAG, "Flash test : ++++++++++ receiveEMREvent() " + x + ", " + y + ",  " + press + ", " + mode + "," + action + ")");
		if(mPointHandler == null) {
            Log.i(TAG, "Flash test : ++++ receiveEMREvent() mPointHandler is null");
            return;
        }

        Message msg = mPointHandler.obtainMessage();
        FlashPoint point = new FlashPoint(x, y, press, mode, action);
        msg.what = MSG_FLAG_TOUCH;
        msg.obj = (Object) point;
        msg.sendToTarget();*/
    }

    public static void forceUpdateJava() {
        //Log.d(TAG, "Flash test : ++++++++++ receiveEMREvent() (x, y, press, acttion) = (" + x + ", " + y + ",  " + press + ", " + action + ")");
        /*Log.d(TAG,"forceUpdateJava");
		if(mPointHandler == null) {
            Log.i(TAG, "Flash test : ++++ forceUpdateJava() mPointHandler is null");
            return;
        }

        Message msg = mPointHandler.obtainMessage();
        msg.what = MSG_FLAG_UPDATE;
        msg.sendToTarget();*/
    }
}
