#error not implemented

#include <good/bitset.h"


namespace good
{


    //************************************************************************************************************
    /// Class for handling matrix of bits.
    //************************************************************************************************************
    template < typename Alloc >
    class base_bitmatrix
    {
    public:
        typedef good::base_bitset<Alloc> row_container_t;  ///< typedef for row container.
        typedef good::vector<row_container_t> container_t; ///< Bit matrix container.

        /// Constructor with optional size argument.
        base_bitmatrix( int size = 0 );

        /// Constructor with optional size argument.
        void set( int x, int y, bool b = true );

    protected:
        container_t m_cContainer;
        int m_iSize;
    };


    template<int size>
    void base_bitmatrix<size>::arrange()
    {
        int front, back;
        front=back=0;
        while (true)
        {
            //find first false one
            while (degreeArray[back])
            {
                back++;
                if (back>=size)
                {
                    return;
                }
            }

            //find first true one
            front=back;
            while (!degreeArray[front])
            {
                front++;
                if (front>=size)
                {
                    return;
                }
            }
            swap(front, back);
            printf("swap %d and %d \n", front, back);
            degreeArray[front]=false;
            degreeArray[back]=true;
        }
    }

    template<int size>
    void base_bitmatrix<size>::swap(int row1, int row2)
    {
        bool hold;
        for (int i=0; i<size; i++)
        {
            //exchange row
            hold=matrix[row1][i];
            matrix[row1][i]=matrix[row2][i];
            matrix[row2][i]=hold;
            //exchange col
            hold=matrix[i][row1];
            matrix[i][row1]=matrix[i][row2];
            matrix[i][row2]=hold;
        }
    }


    template<int size>
    bool base_bitmatrix<size>::first(int deg)
    {
        int total=0;
        for (int i=0; i<size; i++)
        {
            if (matrix[i].count()>=deg)
            {
                total++;
                degreeArray[i]=true;
            }
            else
            {
                degreeArray[i]=false;
            }
        }
        return total>=deg;
    }

    template<int size>
    bool base_bitmatrix<size>::second(int deg)
    {
        int total=0;
        int sub;
        for (int i=0; i<size; i++)
        {
            //if it is the possible candidate
            if (degreeArray[i])
            {
                sub=0;
                //begin testify the candidate
                for (int j=0; j<size; j++)
                {
                    if (matrix[i][j])
                    {
                        if (matrix[j].count()>=deg)
                        {
                            sub++;
                        }
                    }
                }
                //count the number of qualified candidates
                if (sub>=deg)
                {
                    total++;
                    //printf("row[%d] is ok\n", i);
                }
                else
                {
                    degreeArray[i]=false;
                }
            }
        }
        return total>=deg;
    }


    template<int size>
    bool base_bitmatrix<size>::clique(int deg)
    {
        if (first(deg))
        {
            if (second(deg))
            {
                return true;
            }
        }
        return false;
    }

    template<int size>
    bool base_bitmatrix<size>::degree(int row, int deg)
    {
        int result=0;
        if (matrix[row].count()<deg)
        {
            return false;
        }
        for (int i=0; i<size; i++)
        {
            if (matrix[row][i])
            {
                if (matrix[i].count()>=deg)
                {
                    result++;
                }
            }
        }
        return result>=deg-1;
    }




    template<int size>
    void base_bitmatrix<size>::showPower(int row, int times)
    {
        base_bitmatrix<size>* result=this;
        for (int i=0; i<times; i++)
        {
            result=result->power(row);
            printf("show power of row #%d of times %d\n", row, i);
            result->display();

        }
    }



    template<int size>
    void base_bitmatrix<size>::showPower(int times)
    {
        base_bitmatrix<size>* result;
        if (times==0)
        {
            return;
        }
        for (int i=0; i<size; i++)
        {
            result=power(i);

            printf("show power of times:%d, row of %d\n\n", times, i);
            result->display();
            result->showPower(times-1);
        }
    }

    template<int size>
    void base_bitmatrix<size>::update(int row)
    {
        values[row]=matrix[row].to_ulong();
    }

    //the dynamic memory is not handled yet
    template<int size>
    base_bitmatrix<size>* base_bitmatrix<size>::power(int row)
    {
        base_bitmatrix<size>* result=new base_bitmatrix<size>;

        for (int i=0; i<size; i++)
        {
            result->matrix[i].reset();
            result->matrix[i]|=matrix[row];
            result->matrix[i]&=matrix[i];
            result->update(i);
        }
        return result;
    }



    template<int size>
    void base_bitmatrix<size>::randomSet()
    {
        bool result;
        for (int r=0; r<size; r++)
        {
            for (int c=r; c<size; c++)
            {
                if (r==c)
                {
                    result=true;
                }
                else
                {
                    result=rand()%2==0;
                }
                matrix[r].set(size-c-1, result);
                matrix[c].set(size-r-1, result);
            }

        }
        for (int i=0; i<size; i++)
        {
            update(i);
        }
    }


    template<int size>
    void base_bitmatrix<size>::assign(int row, ulong seq)
    {
        bitset<size> B(seq);
        matrix[row].reset();
        matrix[row].operator|=(B);
        values[row]=seq;
    }



    template<int size>
    base_bitmatrix<size>::base_bitmatrix<size>()
    {
        for (int i=0; i<size; i++)
        {
            matrix[i].reset();
        }
    }

    template<int size>
    void base_bitmatrix<size>::set(int row, int col, bool val)
    {
        if (row>=size||col>=size||row<0||col<0)
        {
            printf("out of bound\n");
        }
        else
        {
            matrix[row].set(col, val);
            update(row);
        }
    }



    template<int size>
    void base_bitmatrix<size>::display()
    {
        for (int i=0; i<size; i++)
        {
            printf("%s     (%X)\n", matrix[i].to_string().data(), values[i]);
        }
    }

} // namespace good
