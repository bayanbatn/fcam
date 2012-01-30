package com.nvidia.fcamerapro;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.Arrays;
import java.util.regex.Pattern;

import android.app.ActionBar;
import android.app.ActionBar.Tab;
import android.app.Activity;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.os.Environment;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

/**
 * This is the main application, or "activity" that runs FCamera.
 * FCameraPROActivity class includes all the UI widgets, and uses the singleton
 * FCameraInterface object to issue commands to the C++ thread.
 */
public class FCameraPROActivity extends Activity implements ActionBar.TabListener {
	final private static int TAB_CAPTURE_MONO = 0;
	final private static int TAB_VIEWER = 1;
	private String mStorageDirectory;
	public String getStorageDirectory() { return mStorageDirectory; }

	// These are two views supported by this application. In the UI,
	// the user may switch between the two using tabs.
	private CameraFragment mCameraFragment;
	private ViewerFragment mViewerFragment;
	

	public void onStop() {
		// This kills the background instance.
		System.exit(0);
	}

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		// Create storage directory if necessary.
		try {
			mStorageDirectory = Environment.getExternalStorageDirectory().getCanonicalPath()
					+ File.separatorChar + Settings.STORAGE_DIRECTORY;
			File dir = new File(mStorageDirectory);
			if (!dir.exists() && !dir.mkdirs()) {
				throw new IOException("Unable to create gallery storage!");
			}
		} catch (IOException e) {
			e.printStackTrace();
		}

		// Pass the storage location to the native code (C++ side).
		FCamInterface.GetInstance().setStorageDirectory(mStorageDirectory);

		// Figure out first available stack id. The next photo taken will be
		// saved with this id.
		File dir = new File(mStorageDirectory);
		final Pattern pattern = Pattern.compile(Settings.IMAGE_STACK_PATTERN);
		String[] stackFileNames = dir.list(new FilenameFilter() {
			public boolean accept(File dir, String filename) {
				return pattern.matcher(filename).matches();
			}
		});
		if (stackFileNames != null && stackFileNames.length > 0) {
			Arrays.sort(stackFileNames);
			// Here we extract the largest existing stack id. The code below
			// has knowledge of Settings.IMAGE_STACK_PATTERN, which is not a
			// good design.
			int startIndex = Settings.IMAGE_STACK_PATTERN.indexOf("\\d");
			int lastId = Integer.parseInt(stackFileNames[stackFileNames.length - 1].substring(startIndex, startIndex + 4));
			FCamInterface.GetInstance().setOutputFileId(lastId + 1);
		}

		// We keep a single instance of each fragment despite Android 
		// Fragment API seeming to destroy the fragment after each tab switch
		mCameraFragment = new CameraFragment();
		mViewerFragment = new ViewerFragment();

		// Set up the layout and add tabs. There will be two tabs, one for each fragment.
		setContentView(R.layout.main);
		ActionBar bar = getActionBar();
		bar.addTab(bar.newTab().setText(getResources().getString(R.string.menu_mono_capture))
				.setTabListener(this).setTag(TAB_CAPTURE_MONO));
		bar.addTab(bar.newTab().setText(getResources().getString(R.string.menu_viewer))
				.setTabListener(this).setTag(TAB_VIEWER));
		bar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM | ActionBar.DISPLAY_USE_LOGO);
		bar.setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);
		bar.setDisplayShowHomeEnabled(true);
	}

	// React to user interactions with the tabs.
	public void onTabReselected(Tab tab, FragmentTransaction ft) {}
	public void onTabUnselected(Tab tab, FragmentTransaction ft) {}
	public void onTabSelected(Tab tab, FragmentTransaction ft) {
		switch ((Integer) tab.getTag()) {
		case TAB_CAPTURE_MONO:
			ft.replace(R.id.main_view, mCameraFragment);
			if (ft.isAddToBackStackAllowed()) {
				ft.addToBackStack(null);
			}
			break;
		case TAB_VIEWER:
			ft.replace(R.id.main_view, mViewerFragment);
			if (ft.isAddToBackStackAllowed()) {
				ft.addToBackStack(null);
			}
			break;
		}
		invalidateOptionsMenu();
	}

	// Handle options menu on top-right, if necessary.
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		switch ((Integer) getActionBar().getSelectedTab().getTag()) {
		case TAB_CAPTURE_MONO:
			break;
		case TAB_VIEWER:
			inflater.inflate(R.menu.viewer_menu, menu);
			break;
		}
		return true;
	}
	public boolean onOptionsItemSelected(MenuItem item) {
		return super.onOptionsItemSelected(item);
	}
}
