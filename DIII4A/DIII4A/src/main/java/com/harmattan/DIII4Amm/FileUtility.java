package com.harmattan.DIII4Amm;
import java.io.File;
import java.io.IOException;
import java.io.FileReader;
import java.io.Closeable;
import java.io.FileWriter;

public final class FileUtility
{
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
        try
        {
            if (stream != null)
                stream.close();
                return true;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }
    }
    
    private FileUtility() {}
}
