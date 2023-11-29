package com.karin.idTech4Amm.lib;

import android.util.Log;

import com.karin.idTech4Amm.misc.Function;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.FileReader;
import java.io.Closeable;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;

/**
 * Local file IO utility
 */
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

    public static byte[] ReadStream(InputStream in, int...bufferSizeArg)
    {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        long size = Copy(os, in, bufferSizeArg);
        byte[] res = null;
        if(size >= 0)
            res = os.toByteArray();
        CloseStream(os);
        return res;
    }

    public static String GetFileExtension(String fileName)
    {
        if(null == fileName)
            return null;
        int index = fileName.lastIndexOf(".");
        if(index <= 0 || index == fileName.length() - 1)
            return "";
        return fileName.substring(index + 1);
    }

    public static String GetFileBaseName(String fileName)
    {
        if(null == fileName)
            return null;
        int index = fileName.lastIndexOf(".");
        if(index <= 0 || index == fileName.length() - 1)
            return fileName;
        return fileName.substring(0, index);
    }

    public static boolean mkdir(String path, boolean p)
    {
        File file = new File(path);
        if(p)
            return file.mkdirs();
        else
            return file.mkdir();
    }

    public static long du(String path)
    {
        return du(new File(path));
    }

    public static long du(File file)
    {
        return du(file, null);
    }

    public static long du(String path, Function filter)
    {
        return du(new File(path), filter);
    }

    public static long du(File file, Function filter)
    {
        if(null != filter && !(boolean)filter.Invoke(file))
            return -2L;
        if(!file.exists())
            return -1L;
        if(file.isDirectory())
        {
            long sum = 0;
            File[] files = file.listFiles();
            if(null == files)
                return 0;
            for (File f : files)
            {
                long l = du(f, filter);
                if(l > 0)
                    sum += l;
            }
            return sum;
        }
        else
        {
            return file.length();
        }
    }

    public static String FormatSize(long size)
    {
        String[] Unit = {"Bytes", "K", "M", "G", "T"};
        //const Unit = ["byte", "K", "M", "G", "T"];
        double s;
        int i;
        for(s = size, i = 0; s >= 1024.0 && i < Unit.length - 1; s /= 1024.0, i++);
        s = Math.round(s * 100.0) / 100.0;
        return s + Unit[i];
    }

    public static String RelativePath(String a, String b)
    {
        return RelativePath(new File(a), new File(b));
    }

    public static String RelativePath(File a, File b)
    {
        if(a == b)
            return "";
        String ap = AbsolutePath(a);
        String bp = AbsolutePath(b);
        if(ap.equals(bp))
            return "";
        if(ap.startsWith(bp))
            return ap.substring(bp.length());
        else if(bp.startsWith(ap))
            return bp.substring(ap.length());
        else
            return null;
    }

    public static String AbsolutePath(File a)
    {
        try
        {
            return a.getCanonicalPath();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return a.getAbsolutePath();
        }
    }

    public static boolean mv(String src, String target)
    {
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O)
        {
            try
            {
                Path srcPath = Paths.get(src);
                Path targetPath = Paths.get(target);
                if(Files.isDirectory(srcPath))
                {
                    Files.walk(srcPath).forEach((x) -> {
                        File file = x.toFile();
                        String relativePath = file.getAbsolutePath().substring(src.length());
                        Path targetP = Paths.get(target + relativePath);
                        try
                        {
                            if(Files.isDirectory(x))
                            {
                                Files.createDirectories(targetP);
                            }
                            else
                            {
                                Files.move(x, targetP/*, StandardCopyOption.REPLACE_EXISTING*/);
                            }
                        }
                        catch (IOException e)
                        {
                            throw new RuntimeException(e);
                        }
                    });
                }
                else
                    Files.move(srcPath, targetPath/*, StandardCopyOption.REPLACE_EXISTING*/);
                return true;
            }
            catch (Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }
        else
        {
            File srcFile = new File(src);
            try
            {
                return srcFile.renameTo(new File(target));
            }
            catch (Exception e)
            {
                e.printStackTrace();
                return false;
            }
        }
    }
    
    private FileUtility() {}
}
