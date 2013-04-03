/****************************************************************/
/* Sequential and Parallel Sparse Matrix Multiplication Library */
/* version 2.3 --------------------------------------------------/
/* date: 01/18/2009 ---------------------------------------------/
/* author: Aydin Buluc (aydin@cs.ucsb.edu) ----------------------/
/* description: Helper class in order to get rid of copying  	*/
/*	and temporaries during the A += B*C call		*/
/* ack: This technique is described in Stroustrup, 		*/
/*	The C++ Programming Language, 3rd Edition, 		*/
/*	Section 22.4.7 [Temporaries, Copying and Loops]		*/
/****************************************************************/


#ifndef _MM_MUL_H
#define _MM_MUL_H

template <class BT>
struct MMmul
{
	const BT & sm1;
	const BT & sm2;

	// Constructor 
	MMmul(const BT & smm1, const BT & smm2): sm1(smm1), sm2(smm2) { }

	// No need for operator BT() because we have the corresponding copy constructor 
        // and assignment operators to evaluate and return result !

};

template <typename BT>
inline MMmul< BT > operator* (const BT & smm1, const BT & smm2)
{
	return MMmul< BT >(smm1,smm2);	//! Just defer the multiplication
}

#endif
	
