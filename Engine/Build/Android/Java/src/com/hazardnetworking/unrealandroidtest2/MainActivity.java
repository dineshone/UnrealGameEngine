/*
 
 .----------------.  .----------------.  .-----------------. .----------------.  .----------------.  .----------------.
 | .--------------. || .--------------. || .--------------. || .--------------. || .--------------. || .--------------. |
 | |  ________    | || |     _____    | || | ____  _____  | || |  _________   | || |    _______   | || |  ____  ____  | |
 | | |_   ___ `.  | || |    |_   _|   | || ||_   \|_   _| | || | |_   ___  |  | || |   /  ___  |  | || | |_   ||   _| | |
 | |   | |   `. \ | || |      | |     | || |  |   \ | |   | || |   | |_  \_|  | || |  |  (__ \_|  | || |   | |__| |   | |
 | |   | |    | | | || |      | |     | || |  | |\ \| |   | || |   |  _|  _   | || |   '.___`-.   | || |   |  __  |   | |
 | |  _| |___.' / | || |     _| |_    | || | _| |_\   |_  | || |  _| |___/ |  | || |  |`\____) |  | || |  _| |  | |_  | |
 | | |________.'  | || |    |_____|   | || ||_____|\____| | || | |_________|  | || |  |_______.'  | || | |____||____| | |
 | |              | || |              | || |              | || |              | || |              | || |              | |
 | '--------------' || '--------------' || '--------------' || '--------------' || '--------------' || '--------------' |
 '----------------'  '----------------'  '----------------'  '----------------'  '----------------'  '----------------'
 
 
 
 */



package com.hazardnetworking.unrealandroidtest2;

import android.app.Activity;

import android.app.Dialog;
import android.os.Bundle;
import android.view.View;

public class MainActivity extends Activity {
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_main);
        
        
        
        Dialog mSplashDialog;
        
        try {
            // try to get the splash theme (can't use R.style.UE4SplashTheme since we don't know the package name until runtime)
            int SplashThemeId = getResources().getIdentifier("appTheme", "style", getPackageName());
            mSplashDialog = new Dialog(this, SplashThemeId);
            mSplashDialog.setCancelable(false);
            View decorView = mSplashDialog.getWindow().getDecorView();
            // only do this on KitKat and above
            if(android.os.Build.VERSION.SDK_INT >= 19) {
                decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                                                | View.SYSTEM_UI_FLAG_FULLSCREEN
                                                | View.SYSTEM_UI_FLAG_IMMERSIVE);  // NOT sticky.. will be set to sticky later!
            }
            mSplashDialog.show();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        
    }
}
