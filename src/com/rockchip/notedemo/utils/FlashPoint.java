package com.rockchip.notedemo.utils;

public class FlashPoint {

    //Eraser Mode enable
    public static final int PEN_ERASE_DISABLE	= 0;
    public static final int PEN_ERASE_ENABLE	= 1;

    //Strokes Mode enable
    public static final int PEN_STROKES_DISABLE	= 0;
    public static final int PEN_STROKES_ENABLE	= 1;

    //Action Code
    public static final int ACTION_DOWN 	= 0;
    public static final int ACTION_UP 		= 1;
    public static final int ACTION_MOVE 	= 2;
    public static final int ACTION_CLEAR 	= 3;
    public static final int ACTION_UNDO 	= 4;
    public static final int ACTION_REDO 	= 5;
    public static final int ACTION_UNKOWN 	= -1;

    private static final int PEN_WIDTH_DEFAULT = 3;

    public int x;
    public int y;
    public int press;
	public int mode;
    public int action;

    public FlashPoint(int x, int y, int press, int mode, int action) {
        this.x = x;
        this.y = y;
        this.press = press;
        this.mode = mode;
        this.action = action;
    }
}
