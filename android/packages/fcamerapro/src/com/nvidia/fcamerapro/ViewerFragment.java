package com.nvidia.fcamerapro;

import java.io.File;
import java.io.IOException;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.view.ActionMode;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.BaseAdapter;
import android.widget.Gallery;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

/**
 * ViewerFragment is the UI tab that corresponds to the image gallery.
 * See res/layout/viewer.xml for its layout.
 * 
 * This class also implements a number of listener interface, in order to handle
 * the user interaction with the components.
 *  
 * @see android.app.Fragment
 */
public final class ViewerFragment extends Fragment implements FCamInterfaceEventListener, OnItemLongClickListener, OnItemSelectedListener, OnItemClickListener {
	private View mContentView;
	private Gallery mStackGallery, mImageGallery;
	private HistogramView mHistogram;
	private ImageStackManager mImageStackManager;
	private WebView mLargePreview;
	private View mGalleryPreview;
	private TextView mGalleryInfoLabel, mGalleryInfoValue;
	private Toast mPreviewHint;
	final private Handler mHandler = new Handler();

	private int mSelectedStack;

	private ActionMode.Callback mLargePreviewActionModeCallback = new ActionMode.Callback() {
		public boolean onCreateActionMode(ActionMode actionMode, Menu menu) {
			actionMode.setTitle(R.string.label_large_preview);
			mGalleryPreview.setVisibility(View.INVISIBLE);
			mLargePreview.setVisibility(View.VISIBLE);
			mContentView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN);
			return true;
		}
		public boolean onPrepareActionMode(ActionMode actionMode, Menu menu) { return false; }
		public boolean onActionItemClicked(ActionMode actionMode, MenuItem menuItem) { return false; }
		public void onDestroyActionMode(ActionMode actionMode) {
			mLargePreview.loadUrl("about:blank");
			mLargePreview.setVisibility(View.INVISIBLE);
			mGalleryPreview.setVisibility(View.VISIBLE);
			mContentView.setSystemUiVisibility(View.STATUS_BAR_VISIBLE);
		}
	};

	/* ====================================================================
	 * Methods inherited from Fragment, overridden here.
	 * ==================================================================== */
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.mi_delete:
			if (mImageStackManager.getStackCount() != 0 && mImageStackManager.getStack(mSelectedStack).isLoadComplete()) {
				// Show a confirmation dialog.
				FragmentTransaction ft = getFragmentManager().beginTransaction();
				DialogFragment newFragment = ConfirmationDialogFragment.NewInstance(
						getResources().getString(R.string.menu_item_delete_stack),
						String.format(getResources().getString(R.string.confirmation_delete_stack),
								Utils.GetFileName(mImageStackManager.getStack(mSelectedStack).getName())),
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
								mImageStackManager.getStack(mSelectedStack).removeFromFileSystem();
								if (mSelectedStack != 0 && mSelectedStack == mImageStackManager.getStackCount() - 1) {
									mSelectedStack--;
								}
								mImageStackManager.refreshImageStacks();
							}
						});
				newFragment.show(ft, "dialog");
			}
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		initContentView();
	}

	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		FCamInterface.GetInstance().addEventListener(this);
		// Refresh image stacks upon creation, and move to the beginning of the stack.
		mImageStackManager.refreshImageStacks();
		if (mImageStackManager.getStackCount() != 0) {
			mHandler.post(new Runnable() {
				public void run() {
					mStackGallery.setSelection(0);
				}
			});
		} else updateInfoPage(0, 0); // Empty gallery?	
		mPreviewHint.show();
		return mContentView;
	}

	public void onDestroyView() {
		super.onDestroyView();
		mPreviewHint.cancel();
		FCamInterface.GetInstance().removeEventListener(this);
	}

	public void onDestroy() {
		super.onDestroy();
	}

	/* ====================================================================
	 * Implementation of FCamInterfaceEventListener interface.
	 * ==================================================================== */
	public void onCaptureStart() {}
	public void onCaptureComplete() {}
	public void onFileSystemChange() { mImageStackManager.refreshImageStacks(); }
	public void onShotParamChange(int shotParamId) {}

	/* ====================================================================
	 * A private class for showing a confirmation dialog.
	 * ==================================================================== */
	private static class ConfirmationDialogFragment extends DialogFragment {
		private DialogInterface.OnClickListener mOnConfirmListener;

		public static ConfirmationDialogFragment NewInstance(String title, String text, DialogInterface.OnClickListener onConfirmListener) {
			ConfirmationDialogFragment frag = new ConfirmationDialogFragment();
			Bundle args = new Bundle();
			args.putString("text", text);
			args.putString("title", title);
			frag.setArguments(args);
			frag.mOnConfirmListener = onConfirmListener;
			return frag;
		}

		public Dialog onCreateDialog(Bundle savedInstanceState) {
			String text = getArguments().getString("text");
			String title = getArguments().getString("title");
			return new AlertDialog.Builder(getActivity()).setTitle(title)
					.setMessage(text).setPositiveButton(android.R.string.yes,
							mOnConfirmListener).setNegativeButton(android.R.string.no, null).create();
		}
	}

	/* ====================================================================
	 * Private method helpers.
	 * ==================================================================== */
	// Update the left-hand side metadata pane, to reflect the given image.
	private void updateInfoPage(int stackIndex, int imageIndex) {
		// Do not update fragment UI if it is not attached.
		if (!isVisible()) return;

		String label = getResources().getString(R.string.label_info_page);
		String value = Integer.toString(mImageStackManager.getStackCount());

		if (mImageStackManager.getStackCount() != 0) {
			ImageStack istack = mImageStackManager.getStack(stackIndex);
			label += getResources().getString(R.string.label_info_stack);
			value += "\n" + Utils.GetFileName(istack.getName());
			value += "\n" + istack.getImageCount();

			Image image = null;
			if (imageIndex >= 0 && imageIndex < istack.getImageCount()) {
				image = istack.getImage(imageIndex);
				label += "\n" + getResources().getString(R.string.label_info_image);
				value += "\n\n" + image.getInfo();
			}
			mHistogram.setDataProvider(image);
		} else {
			mHistogram.setDataProvider(null);
		}
		mGalleryInfoLabel.setText(label);
		mGalleryInfoValue.setText(value);
	}

	// Full initialization routine, called upon onCreate().
	private void initContentView() {
		if (mContentView != null) return; // return if already initialized.
		final FCameraPROActivity activity = ((FCameraPROActivity) getActivity());
		
		// Instantiate the stack manager.
		mImageStackManager = new ImageStackManager(activity.getStorageDirectory());

		// Load layout.
		mContentView = activity.getLayoutInflater().inflate(R.layout.viewer, null);
		setHasOptionsMenu(true);

		TypedArray attr = activity.obtainStyledAttributes(R.styleable.Gallery);
		final int itemBackgroundStyleId = attr.getResourceId(R.styleable.Gallery_android_galleryItemBackground, 0);

		// Hooks for the individual UI components in the layout.
		mGalleryInfoLabel = (TextView) mContentView.findViewById(R.id.tv_gallery_info_label);
		mGalleryInfoValue = (TextView) mContentView.findViewById(R.id.tv_gallery_info_value);
		mHistogram = (HistogramView) mContentView.findViewById(R.id.gallery_histogram);
		mPreviewHint = Toast.makeText(activity, R.string.label_preview_hint, Toast.LENGTH_SHORT);
		mGalleryPreview = (View) mContentView.findViewById(R.id.gallery_preview);
		mLargePreview = (WebView) mContentView.findViewById(R.id.gallery_large_preview);
		mLargePreview.setBackgroundColor(Color.BLACK);
		mLargePreview.getSettings().setBuiltInZoomControls(true);
		mLargePreview.getSettings().setUseWideViewPort(true);
		mLargePreview.getSettings().setLoadWithOverviewMode(true);

		// Hook for the stack gallery.
		mStackGallery = (Gallery) mContentView.findViewById(R.id.stack_gallery);
		mStackGallery.setHorizontalFadingEdgeEnabled(true);
		mStackGallery.setAdapter(new BaseAdapter() {
			public int getCount() { return mImageStackManager.getStackCount(); }
			public Object getItem(int position) { return position; }
			public long getItemId(int position) { return position; }
			public View getView(int position, View convertView, ViewGroup parent) {
				View thumbnail;
				if (convertView != null) {
					thumbnail = convertView;
				} else {
					thumbnail = activity.getLayoutInflater().inflate(R.layout.thumbnail_stack, null);
					thumbnail.setBackgroundResource(itemBackgroundStyleId);
				}
				ImageView imageView = (ImageView) thumbnail.findViewById(R.id.thumbnail_image);
				ProgressBar busyBar = (ProgressBar) thumbnail.findViewById(R.id.thumbnail_busy);
				Bitmap bitmap = mImageStackManager.getStack(position).getImage(0).getThumbnail();
				if (bitmap != null) imageView.setImageBitmap(bitmap);

				busyBar.setVisibility(mImageStackManager.getStack(position).isLoadComplete() ? View.INVISIBLE : View.VISIBLE);
				return thumbnail;
			}
		});
		
		// Hook for the image gallery.
		mImageGallery = (Gallery) mContentView.findViewById(R.id.image_gallery);
		mImageGallery.setHorizontalFadingEdgeEnabled(true);
		mImageGallery.setAdapter(new BaseAdapter() {
			public int getCount() {
				return (mImageStackManager.getStackCount() == 0) ? 0 :
					mImageStackManager.getStack(mSelectedStack).getImageCount();
			}
			public void notifyDataSetChanged() {
				super.notifyDataSetChanged();
				updateInfoPage(mSelectedStack, mImageGallery.getSelectedItemPosition());
			}
			public Object getItem(int position) { return position; }
			public long getItemId(int position) { return position; }
			public View getView(int position, View convertView, ViewGroup parent) {
				View thumbnail;
				if (convertView != null) {
					thumbnail = convertView;
				} else {
					thumbnail = activity.getLayoutInflater().inflate(R.layout.thumbnail_image, null);
					thumbnail.setBackgroundResource(itemBackgroundStyleId);
				}
				ImageView imageView = (ImageView) thumbnail.findViewById(R.id.thumbnail_image);
				ProgressBar busyBar = (ProgressBar) thumbnail.findViewById(R.id.thumbnail_busy);
				Bitmap bitmap = mImageStackManager.getStack(mSelectedStack).getImage(position).getThumbnail();
				if (bitmap != null) {
					busyBar.setVisibility(View.INVISIBLE);
					imageView.setImageBitmap(bitmap);
				} else {
					busyBar.setVisibility(View.VISIBLE);
				}
				return thumbnail;
			}
		});

		// Attach self as listeners. See the implementation of the listener
		// interfaces below this method.
		mImageGallery.setOnItemLongClickListener(this);
		mImageGallery.setOnItemSelectedListener(this);
		mStackGallery.setOnItemSelectedListener(this);
		mStackGallery.setOnItemClickListener(this);

		// ImageStackManager should listen to content changes.
		mImageStackManager.addContentChangeListener((BaseAdapter) mStackGallery.getAdapter());
		mImageStackManager.addContentChangeListener((BaseAdapter) mImageGallery.getAdapter());		
	}
	
	/* ====================================================================
	 * Implementation of OnItemLongClickListener interface.
	 * ==================================================================== */
	// Long-clicking on an image gallery item should launch a preview for
	// the selected image.
	public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
		if (parent == mImageGallery) {
			Image image = mImageStackManager.getStack(mSelectedStack).getImage(position);
			if (image.getThumbnail() != null) {
				final FCameraPROActivity activity = ((FCameraPROActivity) getActivity());
				activity.startActionMode(mLargePreviewActionModeCallback);
				try {
					mLargePreview.loadUrl(new File(image.getName()).toURL().toString());
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
			return true;
		}
		return false;
	}

	/* ====================================================================
	 * Implementation of OnItemSelectedListener interface.
	 * ==================================================================== */
	public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
		if (parent == mStackGallery) {
			updateInfoPage(mSelectedStack, mImageGallery.getSelectedItemPosition());
		} else if (parent == mImageGallery) {
			updateInfoPage(mSelectedStack, position);
		}
	}

	public void onNothingSelected(AdapterView<?> parent) {
		if (parent == mStackGallery) {
			updateInfoPage(mSelectedStack, -1);
		} else if (parent == mImageGallery) {
			updateInfoPage(mSelectedStack, -1);
		}	
	}

	/* ====================================================================
	 * Implementation of OnItemClickListener interface.
	 * ==================================================================== */
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		if (parent == mStackGallery) {
			mSelectedStack = position;
			((BaseAdapter) mImageGallery.getAdapter()).notifyDataSetChanged();
		}
	}
}
