package com.karin.idTech4Amm.ui;
import android.widget.ArrayAdapter;
import android.content.Context;
import java.util.List;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import java.util.Collection;

/**
 * Simple ArrayAdapter implements
 */
public abstract class ArrayAdapter_base<T> extends ArrayAdapter<T>
{
    private int m_resource = 0;

    public ArrayAdapter_base(Context context, int resource)
    {
        super(context, resource);
        m_resource = resource;
    }

    public ArrayAdapter_base(Context context, int resource, T[] array)
    {
        super(context, resource, array);
        m_resource = resource;
    }

    public ArrayAdapter_base(Context context, int resource, List<T> list)
    {
        super(context, resource, list);
        m_resource = resource;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent)
    {
        T obj = getItem(position);
        View view;

        if(convertView == null)
            view = LayoutInflater.from(parent.getContext()).inflate(m_resource, parent, false);
        else
            view = convertView;
        return obj != null ? GenerateView(position, view, parent, obj) : null;
    }

    public void Remove(int index)
    {
        int count = getCount();
        int i = index < 0 ? count + index : index;
        if(i < 0 || i >= count)
            return;
        remove(getItem(i));
    }

    public void SetData(T[] array)
    {
        clear();
        addAll(array);
    }

    public void SetData(Collection<T> list)
    {
        clear();
        addAll(list);
    }

    public void Remove(int index, int num)
    {
        int count = getCount();
        int i = index < 0 ? count + index : index;
        if(i < 0 || i >= count)
            return;
        for(int m = 0; m < num; m++)
        {
            if(i >= getCount())
                break;
            remove(getItem(i));
        }
    }

    public void RemoveRange(int index, int end)
    {
        int count = getCount();
        int i = index < 0 ? count + index : index;
        if(i < 0 || i >= count)
            return;
        int len = Math.min(end, count);
        for(int m = i; m < len; m++)
        {
            if(i >= getCount())
                break;
            remove(getItem(i));
        }
    }

    public void Insert(int index, T what)
    {
        int count = getCount();
        int i = index < 0 ? count + index : index;
        if(i < 0)
            insert(what, 0);
        else if(i >= count)
            add(what);
        else
            insert(what, i);
    }

    public void Insert(int index, List<T> list)
    {
        int count = getCount();
        int i = index < 0 ? count + index : index;
        if(i < 0)
        {
            for(int m = 0; m < list.size(); m++)
            {
                insert(list.get(m), m);
            }
        }
        else if(i >= count)
        {
            for(T t : list)
                add(t);
        }
        else
        {
            for(int m = 0; m < list.size(); m++)
            {
                insert(list.get(m), m + i);
            }
        }
    }

    public void Update(int index)
    {
        int count = getCount();
        int i = index < 0 ? count + index : index;
        if(i < 0 || i >= count)
            return;
        T item = getItem(index);
        remove(item);
        insert(item, index);
    }

    public int FindIndex(T data)
    {
        for(int i = 0; i < getCount(); i++)
        {
            if(data == getItem(i))
                return i;
        }
        return -1;
    }

    public abstract View GenerateView(int position, View view, ViewGroup parent, T data);
}
