package com.gaurav.avnc

import android.content.Context
import android.graphics.ImageFormat
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.media.ImageReader
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.util.Log
import android.os.Handler
import android.text.InputType
import android.view.SurfaceView
import android.view.Surface
import android.view.SurfaceHolder
import android.widget.EditText
import android.widget.LinearLayout
import androidx.appcompat.app.AlertDialog
import android.Manifest
import android.content.pm.PackageManager
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts


import androidx.camera.core.CameraProvider
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat



class MainActivity : AppCompatActivity()
{

    // Deklariere die native Methode
    var tryingToConnect: Boolean = false
    external fun nativeSetSurface(surface: Surface)
    private external fun nativeSetCameraSurface(surface: Surface)
    external fun sayHelloFromJNI():String
    external fun createGL();
    external fun nativeCleanup();
    external fun nativeSetServerAddress(addr: String)
    external fun nativeSetServerPort(port: Int)

    private val CAMERA_PERMISSION_REQUEST_CODE = 1001

    private lateinit var cameraDevice: CameraDevice
    private lateinit var imageReader : ImageReader

    private val updateHandler = Handler()
    private val updateRunnable = object : Runnable
    {
        override fun run() {

            //Log.d("Update", "Rendering-Update oder Logik")
            if(tryingToConnect)
            {
                createGL();
            }
            // Wiederhole den Task nach 16 ms (ca. 60 FPS)
            updateHandler.postDelayed(this, 16)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        checkCameraPermission()

        showInputDialog { inputString, port ->
            // Übergib den Wert an native Methode:
            nativeSetServerAddress(inputString)
        }


        val surfaceView = SurfaceView(this)
        setContentView(surfaceView)

        surfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                // Übergibt Surface an Native Code
                nativeSetSurface(holder.surface)
            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                // optional
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
                // optional
            }
        })


        // Lade die native Bibliothek
        System.loadLibrary("native-vnc")

        // Rufe die native Methode auf
        val nativeMsg = sayHelloFromJNI();

        // Gib das Ergebnis in den Log aus
        Log.d("NativeMsg", nativeMsg)
        updateHandler.post(updateRunnable)
    }


    fun checkCameraPermission()
    {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED)
        {
            requestCameraPermission()
        }
        else
        {
            //heißt es gab schin eine permission

            Toast.makeText(this, "Permission already granted", Toast.LENGTH_LONG).show()
        }
    }

    //register permission
    private val requestPermissionLauncher = registerForActivityResult(ActivityResultContracts.RequestPermission())
    {
        isGranted: Boolean ->
        if(isGranted)
        {
            Toast.makeText(this, "Permission allowed", Toast.LENGTH_LONG).show()
        }
        else
        {
            //permission is denied
        }
    }

    private fun requestCameraPermission()
    {
        ///check if the permission was denied
        if(ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.CAMERA))
        {
            requestPermissionLauncher.launch(Manifest.permission.CAMERA);
        }
        else
        {
            //directly ask for permission
            requestPermissionLauncher.launch(Manifest.permission.CAMERA);
        }
    }


    override fun onDestroy() {
        super.onDestroy()
        updateHandler.removeCallbacks(updateRunnable)
    }

    //popup fenster
    fun showInputDialog(onInputReceived: (String, Int) -> Unit)
    {
        val editText = EditText(this);
        editText.hint = "IP-Adresse"

        val editTextPort = EditText(this);
        editTextPort.hint = "Port"
        editTextPort.inputType = InputType.TYPE_CLASS_NUMBER

        val layout = LinearLayout(this)
        layout.orientation = LinearLayout.VERTICAL
        layout.setPadding(50, 40, 50, 10)

        layout.addView(editText);
        layout.addView(editTextPort)

        AlertDialog.Builder(this)
                .setTitle("Daten eingeben :)")
                .setView(layout)
                .setPositiveButton("OK") { _, _ ->
                    val input = editText.text.toString()
                    val inputPort = editTextPort.text.toString();
                    val port = inputPort.toIntOrNull() ?: 5900

                    onInputReceived(input, port)  // Callback aufrufen
                    tryingToConnect = true;
                }
                .setCancelable(false)
                .show()
    }
}