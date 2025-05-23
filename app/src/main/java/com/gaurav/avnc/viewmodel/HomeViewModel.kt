/*
 * Copyright (c) 2020  Gaurav Ujjwal.
 *
 * SPDX-License-Identifier:  GPL-3.0-or-later
 *
 * See COPYING.txt for more details.
 */

package com.gaurav.avnc.viewmodel

import android.app.Application
import androidx.lifecycle.MediatorLiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.switchMap
import com.gaurav.avnc.model.ServerProfile
import com.gaurav.avnc.util.LiveEvent
import com.gaurav.avnc.viewmodel.service.Discovery

class HomeViewModel(app: Application) : BaseViewModel(app) {

    /**
     * [ServerProfile]s stored in database.
     * Depending on the user pref, this list may be sorted by server name.
     */
    val serverProfiles by lazy {
        pref.ui.sortServerList.switchMap {
            if (it) serverProfileDao.getSortedLiveList()
            else serverProfileDao.getLiveList()
        }
    }

    /**
     * Used to find new servers.
     */
    val discovery = Discovery

    /**
     * Used for starting new VNC connections.
     */
    val newConnectionEvent = LiveEvent<ServerProfile>()

    /**
     * This event is used for editing/creating server profiles.
     * Home activity observes this event and starts profile editor when it is fired.
     */
    val editProfileEvent = LiveEvent<ServerProfile>()

    /**
     * Fired when a profile is saved to database.
     * Can be used to highlight the new profile in UI.
     */
    val profileSavedEvent = LiveEvent<ServerProfile>()

    /**
     * Fired when a profile is deleted from database.
     * This is used for notifying the user and potentially undo the deletion.
     */
    val profileDeletedEvent = LiveEvent<ServerProfile>()

    /**
     * Starts new connection to given profile.
     */
    fun startConnection(profile: ServerProfile) = newConnectionEvent.fire(profile)

    fun maybeConnectOnAppStart() = launchMain {
        serverProfileDao.getConnectableOnAppStart().firstOrNull()?.let { startConnection(it) }
    }

    /**************************************************************************
     * Server Discovery
     *
     * To save battery, Discovery is stopped when HomeActivity is in background.
     **************************************************************************/
    private var autoStopped = false

    fun startDiscovery() {
        autoStopped = false
        discovery.start(app)
    }

    fun stopDiscovery() {
        autoStopped = false
        discovery.stop()
    }

    fun autoStartDiscovery() {
        if (pref.server.discoveryAutorun || autoStopped)
            startDiscovery()
    }

    fun autoStopDiscovery() {
        if (discovery.isRunning.value == true) {
            stopDiscovery()
            autoStopped = true
        }
    }


    /**************************************************************************
     * Profile editing/creating
     *
     * These are invoked from UI on user actions. We simply fire [editProfileEvent]
     * with appropriate profile, causing the profile editor to be shown.
     *
     * NOTE: We need to make a copy of given profile because the instance
     * given to [editProfileEvent] can be modified by the editor.
     **************************************************************************/

    fun onNewProfile() = editProfileEvent.fire(ServerProfile())
    fun onNewProfile(source: ServerProfile) = editProfileEvent.fire(source.copy(ID = 0))
    fun onEditProfile(profile: ServerProfile) = editProfileEvent.fire(profile.copy())

    fun onDuplicateProfile(profile: ServerProfile) {
        val duplicate = profile.copy(ID = 0)
        duplicate.name += " (Copy)"
        editProfileEvent.fire(duplicate)
    }

    /**************************************************************************
     * Profile persistence
     *
     * These operations are asynchronous.
     **************************************************************************/

    fun saveProfile(profile: ServerProfile) = launchMain {
        serverProfileDao.save(profile)
        profileSavedEvent.fire(profile)
    }

    fun deleteProfile(profile: ServerProfile) = launchMain {
        serverProfileDao.delete(profile)
        profileDeletedEvent.fire(profile)
    }

    /**************************************************************************
     * Rediscovery Indicator
     *
     * [rediscoveredProfiles] is the intersection of saved & discovered servers.
     *
     * To detect reachable server in [serverProfiles], we could directly 'ping'
     * them, but that has its own issues.
     **************************************************************************/
    val rediscoveredProfiles by lazy {
        pref.server.rediscoveryIndicator.switchMap {
            if (it) prepareRediscoveredProfiles()
            else MutableLiveData(null)
        }
    }

    private fun prepareRediscoveredProfiles() = with(MediatorLiveData<List<ServerProfile>>()) {
        val filter = { saved: List<ServerProfile>?, discovered: List<ServerProfile>? ->
            saved?.filter { s -> discovered?.find { s.host == it.host && s.port == it.port } != null }
        }
        addSource(serverProfiles) { value = filter(it, discovery.servers.value) }
        addSource(discovery.servers) { value = filter(serverProfiles.value, it) }
        this
    }
}