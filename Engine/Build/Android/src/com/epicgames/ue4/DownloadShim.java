package com.epicgames.ue4;

import com.YourCompany.UE4Game.OBBDownloaderService;
import com.YourCompany.UE4Game.DownloaderActivity;


public class DownloadShim
{
	public static OBBDownloaderService DownloaderService;
	public static DownloaderActivity DownloadActivity;
	public static Class<DownloaderActivity> GetDownloaderType() { return DownloaderActivity.class; }
}

