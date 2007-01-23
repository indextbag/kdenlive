/***************************************************************************
                          docclipproject.h  -  description
                             -------------------
    begin                : Thu Jun 20 2002
    copyright            : (C) 2002 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DOCCLIPPROJECT_H
#define DOCCLIPPROJECT_H

#include "docclipbase.h"
#include "docclipref.h"
#include "doctrackbaselist.h"
#include "transitionstack.h"
#include "gentime.h"

class ClipManager;

/**This "clip" consists of a number of tracks, clips, overlays, transitions and effects. It is basically
 capable of making multiple clips accessible as if they were a single clip. The "clipType()" of this clip
  depends entirely upon it's contents. */

class DocClipProject:public DocClipBase {
  Q_OBJECT public:
          DocClipProject(double framesPerSecond, int width, int height);
    ~DocClipProject();

    const GenTime & duration() const;

	/** No descriptions */
    const KURL & fileURL() const;

    virtual QDomDocument toXML();
	/** Returns true if the clip duration is known, false otherwise. */
    virtual bool durationKnown() const;

	/** Returns the frames per second this clip runs at. */
    virtual double framesPerSecond() const;
    
    const int videoHeight() {return m_videoheight;}
    const int videoWidth() {return m_videowidth;}
    
    /** Returns the type of this clip */
    const DocClipBase::CLIPTYPE & clipType() const;

    virtual void removeTmpFile() const {}

	/** Adds a track to the project */
    void slotAddTrack(DocTrackBase * track, int ix = -1);
    void slotDeleteTrack(int ix);

    int trackIndex(DocTrackBase * track);
    uint numTracks() const;
    DocTrackBase *findTrack(DocClipRef * clip) const;
    DocTrackBase *track(uint track);
    bool moveSelectedClips(GenTime startOffset, int trackOffset);
    const DocTrackBaseList & trackList() const;
    void slotClipReferenceChanged();
    void requestProjectClose();

	/** Generates the tracklist for this clip from the xml fragment passed in.*/
    void generateTracksFromXML(const EffectDescriptionList & effectList,
	ClipManager & clipManager, const QDomElement & e);


    virtual QDomDocument generateSceneList(bool addProducers = true, bool rendering = false) const;
    QDomDocument generatePartialSceneList(GenTime start, GenTime end, int prodId) const;
	/** If this is a project clip, return true. Overidden, always true from here. */
    virtual bool isProjectClip() const {
	return true;
    }
	/** Creates a clip from the passed QDomElement. This only pertains to those details
	 *  specific to DocClipProject. This clip should only be created via the clip manager,
	 *  or possibly KdenliveDoc when creating it's project clip..*/
	static DocClipProject *createClip(const
	EffectDescriptionList & effectList,
	ClipManager & clipManager, const QDomElement element);
    // Appends scene times for this clip to the passed vector.
    virtual void populateSceneTimes(QValueVector < GenTime >
	&toPopulate) const;
    // Returns an XML document that describes part of the current scene.
    virtual QDomDocument sceneToXML(const GenTime & startTime,
	const GenTime & endTime) const;

	/** Returns true if the xml passed matches the values in this clip */
    virtual bool matchesXML(const QDomElement & element) const;

	/** Returns true if at least one clip in the project clip is currently selected, false otherwise. */
    int hasSelectedClips();

	/** Returns a clip that is currently selected. Only one clip is returned!
	 * This function is intended for times when you need a "master" clip. but have no preferred
	 * choice. */
    DocClipRef *selectedClip();

    DocClipRefList selectedClipList();

	/** Returns true if this clip refers to the clip passed in. A clip refers to another clip if
	 * it uses it as part of it's own composition. */
    virtual bool referencesClip(DocClipBase * clip) const;

	/** Returns true if the clip referenced is selected in this project clip. Returns false otherwise (including
	it the clip is *not* in the project clip at all. */
    bool clipSelected(DocClipRef * clip);

	/** Returns a list of those refClips owned directly by the project clip that contain references
	 * to the specified clip. */
    DocClipRefList referencedClips(DocClipBase * clip);

	/** Returns true if this clip has a meaningful filesize. */
    virtual bool hasFileSize() const {
	return false;
    }
	/** Returns the filesize, or 0 if there is no appropriate filesize. */
	virtual uint fileSize() const {
	return 0;
    }
	/** Returns true if the specified cliplist can be successfully merged with the track
	views, false otherwise. */
	bool canAddClipsToTracks(DocClipRefList & clips, int track,
	const GenTime & clipOffset) const;
	/** Holds a westley list of all different clips in the document */
    QDomDocumentFragment producersList;
    QDomDocumentFragment virtualProducersList;
    

    public slots:
	/** Check a clip does not exceed its maximum length */
    void fixClipDuration(KURL url, GenTime length);

    /** Return the position of a track in the MLT playlist*/
    int playlistTrackNum(int ix) const;
    /** Return the position of the next video track in the MLT playlist*/
    int playlistNextVideoTrack(int ix) const;

    /** Get the clip located on track at time */
    DocClipRef *getClipAt(int track, GenTime time);

    private slots:
	/** Check that the project length is correct. */
    void slotCheckProjectLength();
    
    signals:
	/** This signal is emitted whenever tracks are added to or removed from the project. */
    void trackListChanged();
	/** Emitted whenever the clip layout changes.*/
    void clipLayoutChanged();
	/** Emitted whenever a clip gets selected. */
    void signalClipSelected(DocClipRef *);
    void signalOpenClip(DocClipRef *);
	/** Emitted when the effect stack of a clip changes */
    void effectStackChanged(DocClipRef *);
	/** Emitted when the length of the project clip changes */
    void projectLengthChanged(const GenTime & length);
    /** This signal is emitted whenever the timeline has changed */
    void documentChanged(DocClipBase *);
    /** This signal is emitted whenever a transition was deleted */
    void deletedClipTransition();
    /** This signal is emmitted when a clip was added/removed so that we update its reference number in the project list view */
    void clipReferenceChanged();
    
  private:
	/** Blocks all track signals if block==true, or unblocks them otherwise. Use when you want
	 *  to temporarily ignore emits from tracks. */
    void blockTrackSignals(bool block);
	/** Returns the number of tracks in this project */
    void connectTrack(DocTrackBase * track);
	/** The number of frames per second. */
    double m_framesPerSecond;
        /** The image width. */
    int m_videowidth;
        /** The image height. */
    int m_videoheight;
	/** Holds a list of all tracks in the project. */
    DocTrackBaseList m_tracks;

    const DocTrackBase & track(uint track) const;

    GenTime m_projectLength;
};

#endif
