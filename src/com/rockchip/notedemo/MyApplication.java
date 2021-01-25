package com.rockchip.notedemo;

import android.app.Application;

import java.util.HashMap;

public class MyApplication  extends Application {
    private HashMap<String, Object> map = new HashMap<String, Object>();

    public void put(String key,Object object){
        map.put(key, object);
    }

    public Object get(String key){
        return map.get(key);
    }
}
