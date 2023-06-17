package com.karin.idTech4Amm.network;

import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.sys.Constants;

import org.json.JSONObject;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public final class CheckUpdate
{
    public static final int CONST_INVALID_RELEASE = -1;

    public String error = "";
    public int release = CONST_INVALID_RELEASE;
    public String update;
    public String version;
    public String apk_url;
    public String changes;

    public boolean CheckForUpdate_github()
    {
        final int TimeOut = 60000;
        HttpsURLConnection conn = null;
        InputStream inputStream = null;
        OutputStream outputStream = null;

        Reset();
        try
        {
            URL url = new URL(Constants.CONST_CHECK_FOR_UPDATE_URL);
            conn = (HttpsURLConnection)url.openConnection();
            conn.setRequestMethod("GET");
            conn.setConnectTimeout(TimeOut);
            conn.setInstanceFollowRedirects(true);
            SSLContext sc = SSLContext.getInstance("TLS");
            sc.init(null, new TrustManager[]{
                    new X509TrustManager() {
                        @Override
                        public X509Certificate[] getAcceptedIssuers() {
                            return new X509Certificate[]{};
                        }
                        @Override
                        public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException
                        { }
                        @Override
                        public void checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException { }
                    }
            }, new java.security.SecureRandom());
            SSLSocketFactory newFactory = sc.getSocketFactory();
            conn.setSSLSocketFactory(newFactory);
            conn.setHostnameVerifier(new HostnameVerifier() {
                @Override
                public boolean verify(String hostname, SSLSession session) {
                    return true;
                }
            });
            conn.setDoInput(true); // 总是读取结果
            conn.setUseCaches(false);
            conn.connect();

            int respCode = conn.getResponseCode();
            if(respCode == HttpsURLConnection.HTTP_OK)
            {
                inputStream = conn.getInputStream();
                byte[] data = FileUtility.ReadStream(inputStream);
                if(null != data && data.length > 0)
                {
                    String text = new String(data);
                    JSONObject json = new JSONObject(text);
                    release = json.getInt("release");
                    update = json.getString("update");
                    version = json.getString("version");
                    apk_url = json.getString("apk_url");
                    changes = json.getString("changes");
                    return true;
                }
                error = "Empty response data";
            }
            else
                error = "Network unexpected response: " + respCode;
        }
        catch(Exception e)
        {
            e.printStackTrace();
            error = e.getMessage();
        }
        finally {
            FileUtility.CloseStream(inputStream);
            FileUtility.CloseStream(outputStream);
        }
        return false;
    }

    public boolean CheckForUpdate()
    {
        return CheckForUpdate_github();
    }

    private void Reset()
    {
        error = "";
        release = CONST_INVALID_RELEASE;
        update = null;
        version = null;
        apk_url = null;
        changes = null;
    }
}
