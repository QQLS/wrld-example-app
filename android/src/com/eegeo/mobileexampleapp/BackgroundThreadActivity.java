// Copyright eeGeo Ltd (2012-2015), All Rights Reserved

package com.eegeo.mobileexampleapp;

import java.io.IOException;
import java.io.InputStream;
import java.util.UUID;

import org.json.JSONException;
import org.json.JSONObject;

import com.eegeo.entrypointinfrastructure.EegeoSurfaceView;
import com.eegeo.entrypointinfrastructure.MainActivity;
import com.eegeo.entrypointinfrastructure.NativeJniCalls;
import com.eegeo.helpers.IRuntimePermissionResultHandler;
import com.microsoft.appcenter.Constants;
import com.microsoft.appcenter.utils.async.AppCenterConsumer;
import com.wrld.widgets.search.WrldSearchWidget;


import android.Manifest;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.core.content.ContextCompat;
import android.util.DisplayMetrics;
import android.view.SurfaceHolder;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.app.SearchManager;

import com.microsoft.appcenter.AppCenter;
import com.microsoft.appcenter.crashes.Crashes;

import com.eegeo.nativecrashmanager.NativeCrashManager;

public class BackgroundThreadActivity extends MainActivity
{
    final public Object screenshotsCompletedLock = new Object();

    private long m_nativeAppWindowPtr;
    private EegeoSurfaceView m_surfaceView;
    private SurfaceHolder m_surfaceHolder;
    private ThreadedUpdateRunner m_threadedRunner;
    private Thread m_updater;
    private String m_hockeyAppId;
    /* The url used if the app is opened by a deep link.
     *  As the app in singleTask this is set in onNewIntent and must be
     *  set to null before for the app pauses.
     */
    private Uri m_deepLinkUrlData;
    private boolean m_rotationInitialised = false;
    private boolean m_locationPermissionRecieved;
    public static final int LOCATION_PERMISSION_REQUEST_CODE = 52;
    private int m_timeInMillisecondsBeforeReducingFramerate = -1;
    private float m_reducedFramerateIntervalSeconds;

    static
    {
        System.loadLibrary("eegeo-mobile-example-app");
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        if ((getIntent().getFlags() & Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT) != 0)
        {
            finish();
            return;
        }

        m_hockeyAppId = readHockeyAppId();
        Constants.loadFromContext(this);
        NativeJniCalls.setUpBreakpad(Constants.FILES_PATH);

        PackageInfo pInfo = null;
        try 
        {
        	pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
        } 
        catch (NameNotFoundException e) 
        {
        	e.printStackTrace();
        }
        final String versionName = pInfo.versionName;
        final int versionCode = pInfo.versionCode;
        
        m_rotationInitialised = !setDisplayOrientationBasedOnDeviceProperties();

        setContentView(R.layout.activity_main);


        Intent intent = getIntent();
        if(intent !=null)
        {
            m_deepLinkUrlData = intent.getData();
        }


        m_surfaceView = (EegeoSurfaceView)findViewById(R.id.surface);
        m_surfaceView.getHolder().addCallback(this);
        m_surfaceView.setActivity(this);

        DisplayMetrics dm = getResources().getDisplayMetrics();
        final float dpi = dm.ydpi;
        final int density = dm.densityDpi;
        final Activity activity = this;

        boolean locationPermissionGranted = ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED;
        m_locationPermissionRecieved = locationPermissionGranted;

        getRuntimePermissionDispatcher().addRuntimePermissionResultHandler(new  IRuntimePermissionResultHandler()
        {
            @Override
            public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
            {
                if(requestCode == LOCATION_PERMISSION_REQUEST_CODE)
                {
                    m_locationPermissionRecieved = true;
                }
            }
        });

        this.getRuntimePermissionDispatcher().hasLocationPermissionsWithCode(LOCATION_PERMISSION_REQUEST_CODE);

        m_threadedRunner = new ThreadedUpdateRunner(false);
        m_updater = new Thread(m_threadedRunner);
        m_updater.start();

        m_threadedRunner.blockUntilThreadStartedRunning();

        runOnNativeThread(new Runnable()
        {
            public void run()
            {
                m_nativeAppWindowPtr = NativeJniCalls.createNativeCode(activity, getAssets(), dpi, density, versionName, versionCode);
                
                if(m_nativeAppWindowPtr == 0)
                {
                    throw new RuntimeException("Failed to start native code.");
                }
            }
        });

        initializeFramerateThrottling();
    }

    public void runOnNativeThread(Runnable runnable)
    {
        m_threadedRunner.postTo(runnable);
    }

    @Override
    protected void onResume()
    {
        updateLastTouchTimeNano();
        super.onResume();
    	if(hasValidHockeyAppId())
    	{
    		registerCrashLogging();
    	}

    	final Activity activity = this;

        AppCenter.getInstallId().thenAccept(new AppCenterConsumer<UUID>() {
            @Override
            public void accept(UUID uuid) {
                if(uuid != null) {
                    NativeCrashManager.handleDumpFiles(activity, m_hockeyAppId, uuid.toString());
                }
            }
        });

    }

    @Override
    protected void onPause()
    {
        super.onPause();
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        if (m_threadedRunner == null)
        {
            return;
        }

        runOnNativeThread(new Runnable()
        {
            public void run()
            {
                NativeJniCalls.stopUpdatingNativeCode();
                m_threadedRunner.flagUpdatingNativeCodeStopped();
            }
        });

        m_threadedRunner.blockUntilThreadHasStoppedUpdatingPlatform();

        NativeJniCalls.destroyApplicationUi();

        runOnNativeThread(new Runnable()
        {
            public void run()
            {
                m_threadedRunner.stop();
                NativeJniCalls.destroyNativeCode();
                m_threadedRunner.destroyed();
            }
        });

        m_threadedRunner.blockUntilThreadHasDestroyedPlatform();

        m_nativeAppWindowPtr = 0;
    }

    @Override
    public void onBackPressed()
    {
        if (!checkLocalBackButtonListeners())
        {
            moveTaskToBack(true);
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        //nothing to do
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        runOnNativeThread(new Runnable()
        {
            public void run()
            {
                m_threadedRunner.stop();
                NativeJniCalls.pauseNativeCode();
            }
        });
    }

    @Override
    public void surfaceChanged(final SurfaceHolder holder, int format, int width, int height)
    {
        m_surfaceHolder = holder;
        m_rotationInitialised = !setDisplayOrientationBasedOnDeviceProperties();
        if (!m_rotationInitialised)
        {
            return;
        }

        runOnNativeThread(new Runnable()
        {
            public void run()
            {
                m_surfaceHolder = holder;
                if(m_surfaceHolder != null)
                {
                    long oldWindow = NativeJniCalls.setNativeSurface(m_surfaceHolder.getSurface());
                    m_threadedRunner.start();
                    releaseNativeWindowDeferred(oldWindow);

                    if(m_deepLinkUrlData != null)
                    {
                        NativeJniCalls.handleUrlOpenEvent(m_deepLinkUrlData.getHost(), m_deepLinkUrlData.getPath(), m_deepLinkUrlData.getEncodedQuery());
                        m_deepLinkUrlData = null;
                    }

                    NativeJniCalls.resumeNativeCode();
                }
            }
        });
    }

    @Override
    public void onNewIntent(Intent intent) {
        m_deepLinkUrlData = intent.getData();
        setIntent(intent);
        if(Intent.ACTION_SEARCH.equals(intent.getAction())) {
            String query = intent.getStringExtra(SearchManager.QUERY);
            if(query != null) {
                WrldSearchWidget searchWidget = (WrldSearchWidget)getFragmentManager().findFragmentById(R.id.search_widget);
                searchWidget.getSearchResults(query, null);
            }
        }
    }

    public void dispatchRevealUiMessageToUiThreadFromNativeThread(final long nativeCallerPointer)
    {
        runOnUiThread(new Runnable()
        {
            public void run()
            {
                NativeJniCalls.revealApplicationUi(nativeCallerPointer);
            }
        });
    }

    public void dispatchUiCreatedMessageToNativeThreadFromUiThread(final long nativeCallerPointer)
    {
        runOnNativeThread(new Runnable()
        {
            public void run()
            {
                NativeJniCalls.handleApplicationUiCreatedOnNativeThread(nativeCallerPointer);
            }
        });
    }

    @Override
    public void onConfigurationChanged(Configuration newConfiguration)
    {
        super.onConfigurationChanged(newConfiguration);
        m_rotationInitialised = true;
    }

    @Override
    public void onScreenshotsCompleted()
    {
        synchronized (screenshotsCompletedLock)
        {
            screenshotsCompletedLock.notifyAll();
        }
    }

    private boolean setDisplayOrientationBasedOnDeviceProperties()
    {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        final int width = displayMetrics.widthPixels;
        final int height = displayMetrics.heightPixels;

    	// Technique based on http://stackoverflow.com/a/9308284 using res/values configuration.
        if(getResources().getBoolean(R.bool.isPhone))
        {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
            final boolean needsRotation = width > height;
            return needsRotation;
        }
        else
        {
        	setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
            final boolean needsRotation = width < height;
            return needsRotation;
        }
    }

    private void initializeFramerateThrottling()
    {
        try
        {
            m_timeInMillisecondsBeforeReducingFramerate = getResources().getInteger(R.integer.timeInMillisecondsBeforeReducingFramerate);
        }
        catch (Resources.NotFoundException exception)
        {
            m_timeInMillisecondsBeforeReducingFramerate = -1;
        }

        int reducedFramerate;

        try
        {
            reducedFramerate = getResources().getInteger(R.integer.reducedFramerate);
        }
        catch (Resources.NotFoundException exception)
        {
            reducedFramerate = 1;
        }

        m_reducedFramerateIntervalSeconds = 1.0f / reducedFramerate;
    }
    
    
    private String readHockeyAppId()
    {    
        try
        {  
            String applicationConfigurationPath = NativeJniCalls.getAppConfigurationPath();
            InputStream is = getAssets().open(applicationConfigurationPath);
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            String jsonStr = new String(buffer, "UTF-8");
            JSONObject json = new JSONObject(jsonStr);
            return json.get("hockey_app_id").toString();
        } catch (IOException e) {
            // Master Ball
        } catch (JSONException e) {
            // Master Ball
        }
        
        return "";
    }

    private boolean hasValidHockeyAppId()
    {
    	return m_hockeyAppId.length() == 36;
    }
    private void registerCrashLogging()
    {
        AppCenter.start(getApplication(), m_hockeyAppId, Crashes.class);

    }

    private class ThreadedUpdateRunner implements Runnable
    {
        private long m_endOfLastFrameNano;
        private boolean m_running;
        private volatile Handler m_nativeThreadHandler;
        private float m_frameThrottleDelaySeconds;
        private boolean m_destroyed;
        private boolean m_stoppedUpdatingPlatformBeforeTeardown;

        public ThreadedUpdateRunner(boolean running)
        {
            m_endOfLastFrameNano = System.nanoTime();
            m_running = false;
            m_destroyed = false;
            m_stoppedUpdatingPlatformBeforeTeardown = false;

            float targetFramesPerSecond = 30.f;
            m_frameThrottleDelaySeconds = 1.f/targetFramesPerSecond;
        }

        synchronized void blockUntilThreadStartedRunning()
        {
            while(m_nativeThreadHandler == null);
        }

        synchronized void blockUntilThreadHasDestroyedPlatform()
        {
            while(!m_destroyed)
            {
                SystemClock.sleep(200);
            }
        }

        synchronized void blockUntilThreadHasStoppedUpdatingPlatform()
        {
            while(!m_stoppedUpdatingPlatformBeforeTeardown)
            {
                SystemClock.sleep(200);
            }
        }

        public void postTo(Runnable runnable)
        {
            m_nativeThreadHandler.post(runnable);
        }

        public void start()
        {
            m_running = true;
        }

        public void stop()
        {
            m_running = false;
        }

        public void destroyed()
        {
            m_destroyed = true;
        }

        void flagUpdatingNativeCodeStopped()
        {
            m_stoppedUpdatingPlatformBeforeTeardown = true;
        }

        public final float GetFrameDelaySeconds(long timeNowNano)
        {
            float delay = m_frameThrottleDelaySeconds;
            final boolean canIdleTimeout = m_timeInMillisecondsBeforeReducingFramerate >= 0;

            if (m_running && canIdleTimeout)
            {
                long nanoDeltaSinceLastTouch = timeNowNano - getLastTouchTimeNano();
                final float deltaMillisecondsSinceLastTouch = (float)((double)nanoDeltaSinceLastTouch / 1e6);

                boolean screenMayAnimateFromCompass = getScreenMayAnimateFromCompass();
                boolean screenMayAnimateFromTouch = deltaMillisecondsSinceLastTouch <= m_timeInMillisecondsBeforeReducingFramerate;
                boolean screenMayAnimate = screenMayAnimateFromCompass || screenMayAnimateFromTouch;
                if (!screenMayAnimate)
                {
                    delay = m_reducedFramerateIntervalSeconds;
                }
            }

            return delay;
        }

        public void run()
        {
            Looper.prepare();
            Handler tmp = new Handler();
            m_nativeThreadHandler = tmp;

            runOnNativeThread(new Runnable()
            {
                public void run()
                {
                    long timeNowNano = System.nanoTime();
                    long nanoDelta = timeNowNano - m_endOfLastFrameNano;
                    final float deltaSeconds = (float)((double)nanoDelta / 1e9);
                    final float delay = GetFrameDelaySeconds(timeNowNano);

                    if(deltaSeconds > delay)
                    {
                        if(m_running && m_locationPermissionRecieved)
                        {
                            NativeJniCalls.updateNativeCode(deltaSeconds);

                            runOnUiThread(new Runnable()
                            {
                                public void run()
                                {
                                    NativeJniCalls.updateUiViewCode(deltaSeconds);
                                }
                            });
                        }
                        else
                        {
                            SystemClock.sleep(200);
                        }

                        m_endOfLastFrameNano = timeNowNano;
                    }
                    else if (m_running)
                    {
                        float remainingDelay = delay - deltaSeconds;
                        final float sleepResolution = 0.015f;

                        if (remainingDelay > sleepResolution)
                        {
                            long sleepMilliseconds = (long)((remainingDelay - sleepResolution) * 1000);
                            SystemClock.sleep(sleepMilliseconds);
                        }
                    }

                    runOnNativeThread(this);
                }
            });

            Looper.loop();
        }
    }

    public void releaseNativeWindowDeferred(final long oldWindow)
    {
        runOnNativeThread(new Runnable() {
            @Override
            public void run() {
                NativeJniCalls.releaseNativeWindow(oldWindow);
            }
        });
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
    }
}
