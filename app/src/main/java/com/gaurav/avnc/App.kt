/*
 * Copyright (c) 2021  Gaurav Ujjwal.
 *
 * SPDX-License-Identifier:  GPL-3.0-or-later
 *
 * See COPYING.txt for more details.
 */

package com.gaurav.avnc

import android.app.Application
import androidx.annotation.Keep
import androidx.appcompat.app.AppCompatDelegate
import com.gaurav.avnc.util.AppPreferences

class App : Application() {

    @Keep
    //lateinit var prefs: AppPreferences
    external fun sayHelloFromJNI(): String

    override fun onCreate() {
        super.onCreate()

        System.loadLibrary("native-vnc")
        val nativeMsg = sayHelloFromJNI();
        println(nativeMsg)
    }



}