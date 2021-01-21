package com.rockchip.notedemo;

//import androidx.appcompat.app.AppCompatActivity;
import android.app.Activity;
import android.Manifest;
import android.content.Context;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.CheckBox;

import com.rockchip.notedemo.utils.Point;

import java.util.ArrayList;
import java.util.List;
import android.os.EinkManager;
import android.widget.CompoundButton;
import android.widget.Toast;
import java.lang.reflect.Method;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";

    private static Context mContext;
    private static Handler mHandler;

    private static NoteView mView;
    private Button mCancelBtn;
    private Button mRedrawBtn;
    private Button mDumpBtn;
    private Button mUndoBtn;
    private Button mClearBtn;
    private CheckBox mEraserCheck;

    private NoteJNI mNativeJNI;

    private static int mScreenH;
    private static int mScreenW;
    private static int mLeft;
    private static int mTop;
    private static int mRight;
    private static int mBottom;
    public static int mInitStatus = 1;

    private final static int mPenMode = 3;//画笔模式
    private final static int mEraserMode = 4;//橡皮擦模式

    private final static int RECOVERY = 0;
    private boolean buttonLock = false;

    public static EinkManager mEinkManager;
    public static String mEinkMode;

    private static ArrayList<Point> mPointList = null;

    public static String getProperty(String key, String defaultValue) {
        String value = defaultValue;
        try {
            Class c = Class.forName("android.os.SystemProperties");
            Method get = c.getMethod("get", String.class, String.class);
            value = (String)(get.invoke(c, key, defaultValue));
        } catch (Exception e) {
            e.printStackTrace();
        }finally {
            return value;
        }
    }

    public static void setProperty(String key, String penDrawMode) {
        try {
            Class c = Class.forName("android.os.SystemProperties");
            Method set = c.getMethod("set", String.class, String.class);
            set.invoke(c, key, penDrawMode);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    private static Runnable mRunnable = new Runnable() {
        public void run() {
            int count = 0;
            while(mView.getHeight() <= 0) {
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                if (count++ > 40) {
                    Log.d(TAG, "Flash test : ++++++++ removeCallbacks");
                    mHandler.removeCallbacks(mRunnable);
                    System.exit(0);
                }
                Log.d(TAG, "Flash test : ++++++++ mView.getHeight() = " + mView.getHeight() + ", count = " + count);
            }
            mLeft = mScreenW;
            mTop = mScreenH - mView.getHeight();
            mRight = 0;
            mBottom = mScreenH;
            mView.initNative(new Rect(mLeft, mTop, mRight, mBottom), "");
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "Flash test : +++++++ onCreate()");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mContext = MainActivity.this;
        if(mEinkManager == null){
            mEinkManager = (EinkManager) mContext.getSystemService(Context.EINK_SERVICE); }
        mEinkMode = mEinkManager.getMode();
        Log.d(TAG, "mEinkMode : " + mEinkMode);
        //去掉标题
        // requestWindowFeature(Window.FEATURE_NO_TITLE);
        //设置竖屏
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        if(mPointList == null) {
            Log.d(TAG, "mPointList clear");
            mPointList = new ArrayList<Point>();
            mPointList.clear();
        }

        MyApplication myApplication = (MyApplication)getApplication();
        ArrayList<Point> pointArrayList = (ArrayList<Point>) myApplication.get("list");
        if(pointArrayList != null) {
            Log.d(TAG, "pointArrayList: " + pointArrayList.size());
        }
        mNativeJNI = new NoteJNI();
        mRedrawBtn = (Button) findViewById(R.id.redraw);
        mRedrawBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mNativeJNI.native_set_is_drawing(1);
                            mNativeJNI.native_redraw(mTop, mRight, mBottom, mLeft);
                            mNativeJNI.native_set_is_drawing(0);
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });
        mCancelBtn = (Button) findViewById(R.id.cancel);
        mCancelBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
				mNativeJNI.native_clear();
                MainActivity.this.finish();
            }
        });
        mUndoBtn = (Button) findViewById(R.id.undo);
        mUndoBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread(){
                    @Override
                    public void run() {
                        if(!buttonLock) {
                            buttonLock = true;
                            //需要在子线程中处理的逻辑
                            mNativeJNI.native_set_is_drawing(1);
                            mNativeJNI.native_undo(mTop, mRight, mBottom, mLeft);
                            mNativeJNI.native_set_is_drawing(0);
                            buttonLock = false;
                        }
                    }
                }.start();
            }
        });
        mClearBtn = (Button) findViewById(R.id.clear);
        mClearBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if(!buttonLock) {
                    buttonLock = true;
                    mNativeJNI.native_clear();
                    buttonLock = false;
                }
            }
        });
        mEraserCheck = (CheckBox) findViewById(R.id.eraser);
        mEraserCheck.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked) {
                    mNativeJNI.native_eraser(mEraserMode);
                } else {
                    mNativeJNI.native_eraser(mPenMode);
                }
            }
        });

        mView = (NoteView) findViewById(R.id.note_view);
        Log.d(TAG, "getHeight: " + mView.getHeight());

        DisplayMetrics metrics = new DisplayMetrics();
        metrics = getApplicationContext().getResources().getDisplayMetrics();
        mScreenW = metrics.widthPixels;
        mScreenH = metrics.heightPixels;

        mHandler = new Handler();
        mHandler.postDelayed(mRunnable, 50);
    }

    @Override
    public void onResume() {
        Log.d(TAG, "Flash test : +++++++ onResume()");
        setProperty("sys.penwritemode.enable", "1");
        //mView.setEnable(false);
        Log.d(TAG, "-------onResume--------mPointList.size: " + mPointList.size());
        super.onResume();
        if(mHandler == null) {
            Log.d(TAG, "-------mHandler is null");
            mHandler = new Handler();
            mHandler.postDelayed(mRunnable, 10);
        }
    }

    @Override
    public void onPause() {
        setProperty("sys.penwritemode.enable","0");
        Log.d(TAG, "Flash test : +++++++ onPause()");
        //mView.setEnable(true);
        Log.d(TAG, "-------onPause-------mPointList.size: " + mPointList.size());
        mHandler.removeCallbacks(mRunnable);
        mHandler = null;
        super.onPause();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "Flash test : +++++++ onDestroy()");
        mView.exitNativeOnly();
        //mView.setEnable(true);
        //mHandler.removeCallbacks(mRunnable);
        System.exit(0);
        mEinkManager.setMode(mEinkMode);
        super.onDestroy();
    }
}
