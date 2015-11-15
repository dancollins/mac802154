package nz.co.dcollins.qr_app;

import android.app.Activity;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;

import android.content.Context;
import android.content.Intent;

import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;


import android.widget.TextView;
import android.widget.LinearLayout;

import android.util.Log;

import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothGattCharacteristic;

import java.util.UUID;
import java.util.List;


public class MainActivity extends Activity
{
    public static final String TAG = "qr_app";

    private static final int INTENT_ENABLE_BT = 2;

    private static final String COORDINATOR_MAC = "00:00:11:33:DC:00";

    private static final UUID PSK_SERVICE_UUID =
        UUID.fromString("0000dc00-0000-1000-8000-00805f9b34fb");
    private static final UUID PSK_CHAR_UUID =
        UUID.fromString("19a7caa0-3d46-11e5-dc01-0002a5d5c51b");
    private static final UUID MAC_CHAR_UUID =
        UUID.fromString("19a7caa0-3d46-11e5-dc02-0002a5d5c51b");

    /* Application state */
    private enum AppState {
        INIT,
        SEARCHING,
        CONNECTING,
        DISCOVERING,
        CONNECTED,
        HAVE_QR,
        MAC_SENT,
        PSK_SENT
    };

    private AppState mState;

    /* Handler to delay state change after sending MAC and PSK */
    private Handler mPskDelayHandler;

    /* These will hold the QR data */
    private String mPsk;
    private String mMac;

    /* Bluetooth stuff */
    private BluetoothAdapter mBluetoothAdapter;
    private boolean mScanning;
    private BluetoothDevice mDevice;

    /* GATT connection info */
    private BluetoothGatt mCoordinatorGatt;
    private BluetoothGattCharacteristic mPskCharacteristic;
    private BluetoothGattCharacteristic mMacCharacteristic;

    private BluetoothGattCallback mGattCallback =
        new BluetoothGattCallback() {
            @Override
            public void onConnectionStateChange(BluetoothGatt gatt, int status,
                                           int newState) {
                if (status == BluetoothGatt.GATT_SUCCESS)
                {
                    if (newState == BluetoothGatt.STATE_CONNECTED)
                    {
                        setState(AppState.DISCOVERING);
                        gatt.discoverServices();
                    }
                    else
                    {
                        Log.d(MainActivity.TAG, "connection lost");
                        mCoordinatorGatt = null;
                        start_le_scan();
                    }
                }
                else
                {
                    Log.d(MainActivity.TAG, "connection state change failed: " +
                          Integer.toString(status));

                    mCoordinatorGatt = null;
                    start_le_scan();
                }
            }

            @Override
            public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                List<BluetoothGattService> services = gatt.getServices();
                /* Find the PSK service */
                for (BluetoothGattService s : services)
                {
                    if (s.getUuid().equals(PSK_SERVICE_UUID))
                    {
                        Log.d(MainActivity.TAG, "found PSK service!");

                        List<BluetoothGattCharacteristic> chars =
                            s.getCharacteristics();
                        for (BluetoothGattCharacteristic c : chars)
                        {
                            if (c.getUuid().equals(PSK_CHAR_UUID)) {
                                Log.d(MainActivity.TAG, "found PSK char");
                                mPskCharacteristic = c;
                            }
                            else if (c.getUuid().equals(MAC_CHAR_UUID)) {
                                Log.d(MainActivity.TAG, "found MAC char");
                                mMacCharacteristic = c;
                            }
                        }
                    }
                }

                if (mPskCharacteristic != null && mMacCharacteristic != null) {
                    setState(AppState.CONNECTED);
                }
            }

            @Override
            public void onCharacteristicWrite(BluetoothGatt gatt,
                                              BluetoothGattCharacteristic characteristic,
                                              int status) {
                if (characteristic.equals(mMacCharacteristic))
                {
                    setState(AppState.MAC_SENT);
                    mPskCharacteristic.setValue(mPsk);
                    mCoordinatorGatt.writeCharacteristic(mPskCharacteristic);
                }
                else if (characteristic.equals(mPskCharacteristic))
                {
                    setState(AppState.PSK_SENT);
                }
            }
        };

    private BluetoothAdapter.LeScanCallback mLeScanCallback =
        new BluetoothAdapter.LeScanCallback() {
            @Override
            public void onLeScan(final BluetoothDevice dev, int rssi,
                                 byte[] scanRecord) {
                /* Gross MAC filter */
                if (dev.getAddress().equals(COORDINATOR_MAC))
                {
                    Log.d(MainActivity.TAG, "found device!");
                    stop_le_scan();

                    setState(AppState.CONNECTING);

                    mDevice = dev;

                    if (mCoordinatorGatt != null)
                        mCoordinatorGatt.close();

                    mCoordinatorGatt =
                        mDevice.connectGatt(MainActivity.this, false,
                                            mGattCallback);
                }
            }
        };

    protected void setState(AppState newState) {
        mState = newState;
        updateUi();
    }

    /* Update the UI to reflect the current state */
    protected void updateUi() {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    TextView txt = (TextView)findViewById(R.id.txt);

                    Log.d(MainActivity.TAG, "app state: " + mState.toString());

                    switch (mState)
                    {
                    case INIT:
                    case SEARCHING:
                        txt.setText("Scanning...");
                        break;

                    case CONNECTING:
                    case DISCOVERING:
                        txt.setText("Connecting...");
                        break;

                    case CONNECTED:
                        txt.setText("Touch to Scan");
                        break;

                    case HAVE_QR:
                    case MAC_SENT:
                        txt.setText("Sharing Key...");
                        break;

                    case PSK_SENT:
                        txt.setText("Key Shared.");

                        /* We need to run this handler on the UI thread! */
                        mPskDelayHandler = new Handler();
                        mPskDelayHandler.postDelayed(new Runnable() {
                                @Override
                                public void run() {
                                    setState(AppState.CONNECTED);
                                }
                            }, 2500);
                        break;

                    default:
                        txt.setText("Please Restart");
                        break;
                    }
                }
            });
    }

    protected void handleData(String qr_text) {
        Log.d(TAG, "Got QR data: " + qr_text);

        if (qr_text == null)
            return;

        /* Ensure the code is the right format '>MAC|secret<' */
        if (!qr_text.matches(">[0-9a-fA-F]{16}\\|.+?<")) {
            Log.d(TAG, "Invalid QR format");
            setState(AppState.CONNECTED);
            return;
        }

        setState(AppState.HAVE_QR);

        String data[] = qr_text.split("\\|");
        mMac = data[0].substring(1);
        mPsk = data[1].substring(0, data[1].length() - 1);

        Log.d(TAG, "mac: " + mMac);
        Log.d(TAG, "psk: " + mPsk);

        mMacCharacteristic.setValue(mMac);
        mCoordinatorGatt.writeCharacteristic(mMacCharacteristic);
    }

    protected void get_qr() {
        Log.d(MainActivity.TAG, "Getting QR...");
        IntentIntegrator integrator =
            new IntentIntegrator(MainActivity.this);
        integrator.initiateScan();
    }

    protected void start_le_scan() {
        if (mBluetoothAdapter == null)
            return;

        Log.d(TAG, "Starting LE scan...");

        setState(AppState.SEARCHING);

        mScanning = true;
        mBluetoothAdapter.startLeScan(mLeScanCallback);
    }

    protected void stop_le_scan() {
        if (mBluetoothAdapter == null)
            return;

        Log.d(TAG, "Stopping LE scan...");

        mScanning = false;
        mBluetoothAdapter.stopLeScan(mLeScanCallback);
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.main);

        setState(AppState.INIT);

        /* Clicking anywhere on the screen will cause us to check if we're
         * connected. If so, then we'll start the QR reader */
        LinearLayout layout = (LinearLayout)findViewById(R.id.layout);
        layout.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (mState == AppState.CONNECTED)
                        get_qr();
                }
            });
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mState == AppState.INIT)
        {
            /* Get the bluetooth adapter, asking the user to enable if required.
             * When we're done we can start an LE scan */
            final BluetoothManager bluetoothManager =
                (BluetoothManager)getSystemService(Context.BLUETOOTH_SERVICE);
            mBluetoothAdapter = bluetoothManager.getAdapter();

            if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
                Intent enableBtIntent =
                    new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, INTENT_ENABLE_BT);
            } else {
                start_le_scan();
            }
        }
    }

    @Override
    public void onPause() {
        super.onPause();

        switch (mState)
        {
        case SEARCHING:
            stop_le_scan();
            setState(AppState.INIT);
            break;

        default:
            break;
        }
    }

    protected void onActivityResult(int requestCode, int resultCode,
                                    Intent data) {
        switch (requestCode) {
        case IntentIntegrator.REQUEST_CODE:
            IntentResult scanResult =
                IntentIntegrator.parseActivityResult(requestCode,
                                                     resultCode,
                                                     data);
            if (scanResult != null) {
                handleData(scanResult.getContents());
            } else {
                Log.d(TAG, "Received scan result: null");
            }
            break;

        case INTENT_ENABLE_BT:
            Log.d(TAG, "Bluetooth enabled? " + resultCode);
            break;

        default:
            Log.d(TAG, "unknown intent request code: " + requestCode);
            break;
        }
    }
}
