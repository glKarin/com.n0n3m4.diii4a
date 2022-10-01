package com.karin.idTech4Amm.lib;

import java.io.File;
import java.io.IOException;
import java.io.FileReader;
import java.io.Closeable;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.OutputStream;

public final class FileUtility
{
    public static final int DEFAULT_BUFFER_SIZE = 8192;

    public static String file_get_contents(String path)
    {
        return file_get_contents(new File(path));
    }
    
    public static String file_get_contents(File file)
    {
        if(!file.isFile() || !file.canRead())
            return null;
            
        FileReader reader = null;
            try
            {
                reader = new FileReader(file);
                int BUF_SIZE = 1024;
                char[] chars = new char[BUF_SIZE];
                int len;
                StringBuilder sb = new StringBuilder();
                while ((len = reader.read(chars)) > 0)
                    sb.append(chars, 0, len);
               return sb.toString();
            }
            catch (IOException e)
            {
                e.printStackTrace();
                return null;
            }
            finally
            {
                CloseStream(reader);
            }
    }

    public static boolean file_put_contents(String path, String content)
    {
        return file_put_contents(new File(path), content);
    }

    public static boolean file_put_contents(File file, String content)
    {        
        FileWriter writer = null;
        try
        {
            writer = new FileWriter(file);
            writer.append(content);
            writer.flush();
            return true;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            CloseStream(writer);
        }
    }
    
    public static boolean CloseStream(Closeable stream)
    {
        if(null == stream)
            return true;
        try
        {
            stream.close();
            return true;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
    }

    public static long Copy(OutputStream out, InputStream in, int...bufferSizeArg) throws RuntimeException
    {
        if(null == out)
            return -1;
        if(null == in)
            return -1;

        int bufferSize = bufferSizeArg.length > 0 ? bufferSizeArg[0] : 0;
        if (bufferSize <= 0)
            bufferSize = DEFAULT_BUFFER_SIZE;

        byte[] buffer = new byte[bufferSize];

        long size = 0L;

        int readSize;
        try
        {
            while((readSize = in.read(buffer)) != -1)
            {
                out.write(buffer, 0, readSize);
                size += readSize;
                out.flush();
            }
        }
        catch (IOException e)
        {
            throw new RuntimeException(e);
        }

        return size;
    }
    
    private FileUtility() {}
}
