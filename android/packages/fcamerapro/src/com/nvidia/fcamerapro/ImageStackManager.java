package com.nvidia.fcamerapro;

import java.io.File;

import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Vector;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

import android.os.Handler;
import android.widget.BaseAdapter;

/**
 * ImageStackManager is a utility class for managing the photograph gallery.
 * 
 * It provides a listener interface, so that UI elements can register themselves
 * as listeners and will be notified when images from the gallery are loaded.
 */
final class ImageStackManager {
	final private String mGalleryDir;
	final private ThreadPoolExecutor mThreadPool = new ThreadPoolExecutor(1, 4, 5, TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());
	final private HashMap<String, Object> mImageStackFilePaths = new HashMap<String, Object>();
	final private ArrayList<ImageStack> mImageStacks = new ArrayList<ImageStack>();
	final private Vector<BaseAdapter> mContentChangeListenters = new Vector<BaseAdapter>();

	// UI thread handler 
	final private Handler mHandler = new Handler();

	public ImageStackManager(String galleryDir) {
		mGalleryDir = galleryDir;
	}

	/* ====================================================================
	 * Handle listeners that are listening to the associated dataset.
	 * ==================================================================== */
	public void addContentChangeListener(BaseAdapter listener) {
		if (mContentChangeListenters.contains(listener))
			throw new IllegalArgumentException("listener already registered!");
		mContentChangeListenters.add(listener);
	}

	public void removeContentChangeListener(FCamInterfaceEventListener listener) {
		if (!mContentChangeListenters.contains(listener))
			throw new IllegalArgumentException("listener not registered!");
		mContentChangeListenters.remove(listener);
	}

	public void notifyContentChange() {
		mHandler.post(new Runnable() {
			public void run() {
				for (BaseAdapter listener : mContentChangeListenters)
					listener.notifyDataSetChanged();
			}
		});
	}

	/* ====================================================================
	 * Getters
	 * ==================================================================== */
	public String getStorageDirectory() { return mGalleryDir; }

	public ImageStack getStack(int id) {
		return mImageStacks.get(id);
	}

	public int getStackCount() {
		return mImageStacks.size();
	}
	
	/* ====================================================================
	 * Reload the stacks by synchronizing them with file system.
	 * ==================================================================== */
	public synchronized void refreshImageStacks() {
		// Get the list of XML files in the gallery directory.
		File dir = new File(mGalleryDir);
		final Pattern pattern = Pattern.compile(Settings.IMAGE_STACK_PATTERN);
		String[] stackFileNames = dir.list(new FilenameFilter() {
			public boolean accept(File dir, String filename) {
				return pattern.matcher(filename).matches();
			}
		});
		if (stackFileNames == null) {
			// Need zero-sized array for stack remove notification
			stackFileNames = new String[0];
		}
		Arrays.sort(stackFileNames);

		try {
			// Check out for deleted stacks.
			HashMap<String, Object> stackFilePaths = new HashMap<String, Object>(stackFileNames.length);
			for (String fname : stackFileNames) {
				String filePath = mGalleryDir + File.separatorChar + fname;
				stackFilePaths.put(filePath, null);
			}
			boolean notifyContentChange = false;
			for (int i = 0; i < mImageStacks.size(); i++) {
				String imageStackFilePath = mImageStacks.get(i).getName();
				if (!stackFilePaths.containsKey(imageStackFilePath)) {
					// No file. Remove the image stack from the UI.
					mImageStackFilePaths.remove(imageStackFilePath);
					mImageStacks.remove(i);
					notifyContentChange = true;
					i--;
				}
			}
			// Register new stacks.
			for (int i = stackFileNames.length - 1; i >= 0; i--) {
				String filePath = mGalleryDir + File.separatorChar + stackFileNames[i];
				if (!mImageStackFilePaths.containsKey(filePath)) {
					mImageStackFilePaths.put(filePath, null);
					mImageStacks.add(stackFileNames.length - 1 - i, new ImageStack(filePath, this));
				}
			}
			// Notify listeners if some stacks ahve been removed.
			if (notifyContentChange)
				notifyContentChange();
			// Queue tasks for loading stacks.
			for (ImageStack stack : mImageStacks)
				if (!stack.isLoadComplete() && !mThreadPool.getQueue().contains(stack))
					mThreadPool.execute(stack);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
