package com.rockchip.notedemo;


import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Build;
import android.os.CountDownTimer;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import com.rockchip.notedemo.utils.Point;

//import androidx.annotation.RequiresApi;

import com.rockchip.notedemo.utils.FlashPoint;

import java.util.ArrayList;

public class NoteView extends View {
    private static final String TAG = "NoteView";

    private static final boolean DEBUG = true;

    private int mPenWidth = PEN_WIDTH_DEFAULT;

    private boolean mIsNativeInited = false;
    private boolean mIsNativeExited = false;
    private boolean mIsDrawReady = false;
    private boolean mIsWrited = false;
	
	public static final int TOUCH_EVENT 	= 1001;

    private static final int DISPLAY_MODE_NDK 	= 0;
    private static final int DISPLAY_MODE_JAVA 	= 1;

    private static final int PEN_WIDTH_DEFAULT = 3;

    private NoteJNI mNativeJNI;
    private Context mCtx;


    private static ArrayList<FlashPoint> mPointList;

    public NoteView(Context context) {
        super(context);
		Log.d(TAG, "NoteView1");
        mCtx = context;
        initView();
    }

    public NoteView(Context context, AttributeSet attrs) {
        super(context, attrs);
		Log.d(TAG, "NoteView2");
        mCtx = context;
        initView();
    }

    private void initView() {
        mNativeJNI = new NoteJNI();
        invalidate();
    }

    /*
     * 初始化jni相关
     * 参数 rect:View边界距离屏幕的距离值px
     * 参数 bkPng:设置手写背景
     */
    public int initNative(Rect rect, String file_path) {
        Log.d(TAG, "Flash test : +++++++++ initNative() rect = " + rect);
        int initStatus = init(rect, file_path);
        return initStatus;
    }

    private int init(Rect rect, String file_path) {
		int initStatus = mNativeJNI.init(rect, file_path);
        invalidate();
        return initStatus;
    }


    public void setEnable(boolean status) {
		Log.d(TAG, "status :" + status);
        mNativeJNI.native_enable(status ? 1 : 0);
        //if (status) setSleepMode(true);
    }

    public void exitNativeOnly() {
        mNativeJNI.native_exit();
    }

    public void exit() {
    }

}
