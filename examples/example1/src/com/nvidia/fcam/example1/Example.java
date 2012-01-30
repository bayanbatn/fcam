package com.nvidia.fcam.example1;

import android.app.Activity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;


public class Example extends Activity
{
	/* A handler for msgs generated from the FCam native thread */
	private Handler hNotificationHandler;
	private final static int COMPLETED = 0x1;			// Completion notification
	
	private TextView tv; 
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        
        /* Create the handler that allows us to process messages
         * from a different thread. */
        hNotificationHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
                case COMPLETED:
                	tv.setText(tv.getText() + "FCam example done!\nClick the text to finish\n\n\n\n\n\n\n");
                    tv.setOnClickListener(new OnClickListener() {
                    	public void onClick(View v) {
                    		finish();
                    	}
                    });
                    break;
            }
        }
        };
        
        /* Create a TextView and set its content.
         * the text is retrieved by calling a native
         * function.
         */
        tv = new TextView(this);
        tv.setText("Starting FCam example...");
        setContentView(tv);
        
        // invoke the native code - which will launch
        // its own thread.
        run();
    }
    
    /* onCompletion is called from the native code when the
     * example code has finished executing.
     */
    public void onCompletion()
    {
    	hNotificationHandler.sendEmptyMessage(COMPLETED);
    }
    
    /* A native method that runs the fcam API code and
     * that is included in the 'fcam_example' native library, 
     * which is packaged with this application.
     */
    public native int run();
    
    

    /* Load the required shared libraries.
     * startup. 
     */
    static {
    	System.loadLibrary("FCamTegraHal");
    	System.loadLibrary("fcam_example1");
    }

};