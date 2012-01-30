package com.nvidia.fcamerapro;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

/**
 * ImageStack represents a stack of images that are taken in a burst.
 * For the purpose of the assignment, all images will be a single shot,
 * but this class exists for future extensibility.
 */
public final class ImageStack implements Runnable {
	final private ImageStackManager mOwner;
	final private String mFileName;
	final private ArrayList<Image> mImages = new ArrayList<Image>();
	private boolean mLoadComplete;

	/* ====================================================================
	 * Constructs a new ImageStack by reading the provided XML file.
	 * ==================================================================== */
	public ImageStack(String fileName, ImageStackManager owner) throws IOException {
		mOwner = owner;
		mFileName = fileName;
		mLoadComplete = false;
		// Parse xml
		try {
			SAXParserFactory factory = SAXParserFactory.newInstance();
			SAXParser saxParser = factory.newSAXParser();
			DefaultHandler handler = new DefaultHandler() {
				public void startElement(String uri, String localName, String qname, Attributes attributes) throws SAXException {
					if (qname.equals("image")) {
						mImages.add(new Image(mOwner.getStorageDirectory(), attributes));
					}
				}
				public void endElement(String uri, String localName, String qName) throws SAXException {}
				public void characters(char ch[], int start, int length) throws SAXException {}
			};
			saxParser.parse(new File(fileName), handler);
		} catch (SAXException e) {
			e.printStackTrace();
		} catch (ParserConfigurationException e) {
			e.printStackTrace();
		}
	}
	
	/* ====================================================================
	 * Delete this stack from the file system.
	 * ==================================================================== */
	// The instance's internal state may be inconsistent after deletion.
	public void removeFromFileSystem() {
		for (Image image : mImages) {
			new File(image.getThumbnailName()).delete();	
			new File(image.getName()).delete();	
		}
		new File(mFileName).delete();
	}
	
	/* ====================================================================
	 * Getters 
	 * ==================================================================== */
	public String getName() { return mFileName; }

	public boolean isLoadComplete() { return mLoadComplete; }

	public int getImageCount() { return mImages.size(); }

	public Image getImage(int index) { return mImages.get(index); }

	/* ====================================================================
	 * Implementation of Runnable interface.
	 * ==================================================================== */
	// Running this instance will cause all the thumbnails to be loaded.
	// It is implemented as a Runnable because it makes managing tasks
	// easier for ImageStackManager.
	public void run() {
		boolean loadComplete = true;
		for (Image image : mImages) {
			if (image.getThumbnail() == null) {
				if (!image.loadThumbnail()) {
					loadComplete = false;
					break;
				}
			}
		}
		mLoadComplete = loadComplete;
		mOwner.notifyContentChange();
	}
}
