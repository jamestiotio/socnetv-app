/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.4
 Written in Qt

                          matrix.h  -  description
                             -------------------
    copyright            : (C) 2005-2014 by dimitris kalamaras
    email                : dimitris.kalamaras@gmail.com
 ***************************************************************************/


/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/


#ifndef MATRIX_H
#define MATRIX_H


#include <QtGlobal>		//used for qDebug function
#include <QDebug>
#include <QTextStream>

using namespace std; //or else compiler groans for nothrow

class Row {
public:
    Row (int Actors=0) {
        cell=new (nothrow) float [m_Actors=Actors];
		Q_CHECK_PTR( cell );
        for (register int i=0;i<m_Actors; i++) {
            cell[i]=0;
        }
		m_outEdges=0;
	}

    ~Row() { m_Actors=0, m_outEdges=0 ; delete [] cell;}
	
    Row& operator =(Row & a) {
        if (this != &a){
            if (a.m_Actors!=m_Actors) {
                delete [] cell;
                cell=new (nothrow) float[m_Actors=a.m_Actors];
                Q_CHECK_PTR( cell);
            }
            for (int i=0;i<m_Actors; i++) {
                    cell[i]=a.cell[i];
            }
        }
        return *this;
    }
	
	float column ( int c ) const {
		return cell[c];
	}
	
	void setColumn (int index, float elem) {
		cell[index]=elem;
		if (elem!=0)
			m_outEdges++;
	}

	void clearColumn(int index){
		if (cell[index]!=0)
			m_outEdges--;
		cell[index]=0;
	}	


	//FIXME 
	void updateOutEdges(){
		m_outEdges=0;
		for (int i=0;i<m_Actors; i++) { 
			if (cell[i])
			    m_outEdges++;
		}
	}

	void resize(int Actors) {
		delete [] cell;
        cell=new (nothrow) float[m_Actors=Actors];
		Q_CHECK_PTR( cell);
		for (int i=0;i<m_Actors; i++) { 
			cell[i]=0;
		}
		m_outEdges=0;
	}
	
	void setSize(int Actors){
		m_Actors=Actors;
		//FIXME Matrix.row setSize m_outEdges should be zero
	}
	int outEdges () { return m_outEdges;}

private:
	float *cell;
	int m_Actors, m_outEdges;
};




class Matrix {
public:
    Matrix (int Actors=0) ; /**default constructor - default size 0 */

    Matrix(const Matrix &b) ;	/* Copy constructor allows Matrix a=b  */

    ~Matrix();

    void clear();

    void resize (int Actors) ;

    float item( int r, int c ) ;

    void setItem( int r, int c, float elem );

    void clearItem( int r, int c ) ;

    int cols() {return m_Actors;}

    int rows() {return m_Actors;}

    int size() { return m_Actors;}

    void findMinMaxValues(float&,float&);

    void deleteRowColumn(int i);	/* deletes row i and column i */

    int edgesFrom(int Actor);

    int edgesTo(int Actor);

    int totalEdges();

    bool printMatrixConsole();

    void identityMatrix (int);

    void zeroMatrix (int);

    void fillMatrix (float value );

    Matrix& operator =(Matrix & a);

    Matrix& operator +(Matrix & b);

    Matrix operator *(Matrix & b);	    // undefined

    float  operator ()  (const int r, const int c) ;

    friend QTextStream& operator <<  (QTextStream& os, Matrix& m);

    Matrix & product( Matrix &a, Matrix & b, bool symmetry) ;

    Matrix & productSym( Matrix &a, Matrix & b)  ;

    Matrix & pow (int power, bool symmetry)  ;

    Matrix& subtractFromI () ;

    Matrix& sum (Matrix &a, Matrix &b) ;

    Matrix& inverseByGaussJordanElimination(Matrix &a);

    void swapRows(int rowA,int rowB);		/* elementary matrix algebra */
    void multiplyRow(int row, float value);		/* Multiply every elememt of row A by value */

private:
    Row *row;
    int m_Actors;
};





#endif