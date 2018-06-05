// derived from Oculus Mobile SDK template, (c) 2014 Oculus VR, LLC. All Rights reserved.
package net.mimosa_pudica.ovrvnc;

import android.os.Bundle;
import android.os.Environment;
import android.content.Intent;
import com.oculus.vrappframework.VrActivity;


public class MainActivity extends VrActivity {
	static {
		System.loadLibrary( "ovrapp" );
	}

	public static native long nativeSetAppInterface( VrActivity activity, String packageName, String command, String uri, String extPath );

	@Override
	protected void onCreate( Bundle savedInstanceState ) {
		super.onCreate( savedInstanceState );

		Intent intent = getIntent();
		String packageName = VrActivity.getPackageStringFromIntent( intent );
		String command     = VrActivity.getCommandStringFromIntent( intent );
		String uri         = VrActivity.getUriStringFromIntent( intent );
		String extPath     = Environment.getExternalStorageDirectory().getAbsolutePath();

		setAppPtr( nativeSetAppInterface( this, packageName, command, uri, extPath ) );
	}
}
