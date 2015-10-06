#ifndef LB_BB_H
#define LB_BB_H
/* blackbox/BB.h
 * blackbox base class
 *
 * Functions may take BB<Field> parameters and be separately compiled.
 *
 * Non-template functions are pure virtual.  Code bloat is avoided.
 * Template functions select on a BBType tag.  This introduces some code bloat.
 */
#include <iostream>
#include "linbox/util/error.h"
#include "linbox/matrix/dense-matrix.h"
#include "linbox/matrix/matrix-domain.h"
namespace LinBox {

// for now, only the FIBB tags.
enum BBType {diagonal, permutation, triangular, product, lqup, pluq, other};

template <class Ring>
struct BB 
{
	typedef Ring Field;
	typedef DenseMatrix<Field> ResizableMatrix;
	//typedef DenseMatrix<Field, std::vector<typename Ring::Element> > ResizableMatrix;
	typedef DenseMatrix<Field> Matrix;

	virtual ~BB(){}

	virtual BBType bbTag() const 
	= 0;
	virtual size_t rowdim() const
	= 0;
	virtual size_t coldim() const
	= 0;
	virtual const Field& field() const
	= 0;
	virtual std::ostream& write(std::ostream& os) const
	= 0;
	virtual std::istream& read(std::istream& os) 
	= 0;
	virtual Matrix& applyLeft(Matrix& Y, const Matrix& X) const
	= 0;
	virtual Matrix& applyRight(Matrix& Y, const Matrix& X) const
	= 0;
	template<class OutVector, class InVector>
	OutVector& apply(OutVector& y, const InVector& x) const
	{ switch (bbTag()) 
	  {	//case BBx_tag: static_cast<BBx<Field>*>(this)->apply(y,x); break;
	  	//case permutation: static_cast<Permutation<Field>*>(this)->apply(y, x)
	  	default: throw LinboxError("indirect call to apply not supported for BBType " /* bbTag*/);
	  } 
	  return y;
	}
	template<class OutVector, class InVector>
	OutVector& applyTranspose(OutVector& y, const InVector& x) const
	{ switch (bbTag()) 
	  {	//case BBxtag: static_cast<BBx<Field>*>(this)->applyTranspose(y,x); break;
	  	default: throw LinboxError("indirect call to applyTranspose not supported for BBType " /* bbTag*/);
	  } 
	  return y;
	}

	template<typename BB2>
	void map(BB2& A)
	{ switch (bbTag()) 
	  {	//case bbxtag: static_cast<bbx<Field>*>(this)->map(A); break;
	     // using it's struct rebind;
	  	default: throw LinboxError("indirect call to map not supported for BBType " /* bbTag*/);
	  }
	  return A;
	}

}; // class BB

} // LinBox
#endif // LB_BB_H

// blackbox/fibb.h
#ifndef LB_FIBB_H
#define LB_FIBB_H
/*
FIBB: Fast Inverse BlackBox

The FIBB functions are those of a blackbox plus rank, det, and the solvers: 
Solve, NullSpaceRandom, NullSpaceBasis. 
The solvers have left and right forms.

THe FIBBs are Diagonal, Permutation, Triangular, and products of FIBBs in which one or both are nonsingular.
*/
namespace LinBox{

template <class Ring>
struct FIBB : public BB<Ring> 
{
	using Field = Ring;
	using Element = typename Ring::Element;
	using ResizableMatrix = DenseMatrix<Field>;
	using Matrix = DenseMatrix<Field>;

//	virtual const Field& field() const = 0;

	virtual ~FIBB(){}

	virtual size_t& rank(size_t& r) const
	= 0;

	virtual Element& det( Element& d) const
	= 0;

	// solveRight and solveLeft
	/** @brief Y: AY = X, for this A.
		Solve nonsingular or consistent singular system.  
		If it is consistent singular, an arbitrary solution is provided.  
		X and Y must have identical shape.

		Note that Y+Z is a random sample of the solution space after
		{solveRight(Y, X); nullspaceRandomRight(Z);}.

		Behaviour is unspecified for inconsistent systems (see solveMP).
	*/
	virtual Matrix& solveRight(Matrix& Y, const Matrix& X) const 
	= 0;
	/// Y: YA = X, for this A
	virtual Matrix& solveLeft(Matrix& Y, const Matrix& X) const 
	= 0;

	/// N: AN = 0, each col random.
	virtual Matrix& nullspaceRandomRight(Matrix& N) const 
	= 0;
	/// N: NA = 0, each row random.
	virtual Matrix& nullspaceRandomLeft(Matrix& N) const 
	= 0;
	// this generic is virtual so that it may be specialized for performance

	// nullspaceBasisRight and nullspaceBasisLeft

	/** B: columns are a right nullspace basis for this A.
		
		B is resized and filled so that:
		(1) AB = 0, (2) Ax = 0 => exists y: x = By, and (3) B has full rank.
	*/
	virtual ResizableMatrix& nullspaceBasisRight(ResizableMatrix& B) const 
	= 0;
	/// BA= 0 and xA = 0 => exists y: x = yB and B full rank.
	virtual ResizableMatrix& nullspaceBasisLeft(ResizableMatrix& B) const 
	= 0;
}; // class FIBB

/// N: AN = 0, each col random.
template<class Field>
DenseMatrix<Field>& genericNullspaceRandomRight(DenseMatrix<Field>& N, const FIBB<Field>& A)
//DenseMatrix<Field, std::vector<typename Field::Element> >& genericNullspaceRandomRight(DenseMatrix<Field, std::vector<typename Field::Element> >& N, const FIBB<Field>& A)
{	typedef DenseMatrix<Field> ResizableMatrix;
	typedef DenseMatrix<Field> Matrix;
	ResizableMatrix Xb(A.field(), N.rowdim(), N.coldim());
	ResizableMatrix Yb(A.field(), A.rowdim(), N.coldim());
	Matrix X(Xb); X.random();
	Matrix Y(Yb); 
	A.applyRight(Y, X); // Y = AX
	A.solveRight(N, Y); // AN = AX
	BlasMatrixDomain<Field> MD(A.field());
	return MD.subin(N, X);
}

/// N: NA = 0, each row random.
template<class Field>
DenseMatrix<Field>& genericNullspaceRandomLeft(DenseMatrix<Field>& N, const FIBB<Field>& A)
//DenseMatrix<Field, std::vector<typename Field::Element> >& genericNullspaceRandomLeft(DenseMatrix<Field, std::vector<typename Field::Element> >& N, const FIBB<Field>& A)
{	typedef DenseMatrix<Field> ResizableMatrix;
	typedef DenseMatrix<Field> Matrix;
	ResizableMatrix Xb(A.field(), N.rowdim(), N.coldim());
	ResizableMatrix Yb(A.field(), N.rowdim(), A.coldim());
	Matrix X(Xb); X.random();
	Matrix Y(Yb); 
	A.applyLeft(Y, X); // Y = XA
	A.solveLeft(N, Y); // NA = XA
	BlasMatrixDomain<Field> MD(A.field());
	return MD.subin(N, X);
}

} // namespace LinBox
#endif // LB_FIBB_H

// blackbox/FIBBProduct.h
// todo: possiby merge with compose blackbox
#ifndef LB_FIBBProduct_H
#define LB_FIBBProduct_H
namespace LinBox{

template<class Field_> 
struct FIBBProduct : public FIBB<Field_> { // Fast Inverse BlackBox
	typedef Field_ Field;
	typedef FIBB<Field> Father_t;
  protected: 
    const Father_t* Ap;
    const Father_t* Bp;
	bool allocA; // true only if new used within init
	bool allocB; // true only if new used within init
  public:
    using Element = typename Father_t::Element;
    using ResizableMatrix = typename Father_t::ResizableMatrix;
    using Matrix = typename Father_t::Matrix;

	/* Blackbox functions */
	BBType bbTag() const { return product; }
	size_t rowdim() const;
	size_t coldim() const;
	const Field& field() const;
	Matrix& applyRight(Matrix& Y, const Matrix& X) const; // Y = AX
	Matrix& applyLeft(Matrix& Y, const Matrix& X) const; // Y = XA
	// todo: rebind - hom support
	std::istream& read(std::istream& is);
	std::ostream& write(std::ostream& os) const;

	/* fibb functions */
	size_t& rank(size_t& r) const;

	Element& det(Element& d) const;

	Matrix& solveRight(Matrix& Y, const Matrix& X) const; 
	/// Y: YA = X, for this A
	Matrix& solveLeft(Matrix& Y, const Matrix& X) const; 

	/// N: AN = 0, each col random.
	Matrix& nullspaceRandomRight(Matrix& N) const; 
	/// N: NA = 0, each row random.
	Matrix& nullspaceRandomLeft(Matrix& N) const; 

	// nullspaceBasisRight and nullspaceBasisLeft
	/** B: columns are a right nullspace basis for this A.
		
		B is resized and filled so that:
		(1) AB = 0, (2) Ax = 0 => exists y: x = By, and (3) B has full rank.
	*/
	DenseMatrix<Field>& nullspaceBasisRight(DenseMatrix<Field>& B) const; 
	/// BA= 0 and xA = 0 => exists y: x = yB and B full rank.
	DenseMatrix<Field>& nullspaceBasisLeft(DenseMatrix<Field>& B) const; 

	/* cstors, dstor, initializers */
	FIBBProduct();
	FIBBProduct(const FIBB<Field>& A1, const FIBB<Field>& A2);
	FIBBProduct(const FIBB<Field>& A1, const FIBB<Field>& A2, 
				const FIBB<Field>& A3);
	FIBBProduct(const FIBB<Field>& A1, const FIBB<Field>& A2, 
				const FIBB<Field>& A3, const FIBB<Field>& A4);
	FIBBProduct(const FIBB<Field>& A1, const FIBB<Field>& A2, 
				const FIBB<Field>& A3, const FIBB<Field>& A4, 
				const FIBB<Field>& A5);
	~FIBBProduct(){ if (allocA) delete Ap; if (allocB) delete Bp; }
	public:
	FIBBProduct& init(const FIBB<Field>& A1, const FIBB<Field>& A2);
	FIBBProduct& init(const FIBB<Field>& A1, const FIBB<Field>& A2, 
					  const FIBB<Field>& A3);
	FIBBProduct& init(const FIBB<Field>& A1, const FIBB<Field>& A2, 
					  const FIBB<Field>& A3, const FIBB<Field>& A4);
	FIBBProduct& init(const FIBB<Field>& A1, const FIBB<Field>& A2, 
					  const FIBB<Field>& A3, const FIBB<Field>& A4, 
					  const FIBB<Field>& A5);

}; // class FIBBProduct
}// namespace LinBox
#endif // LB_FIBBProduct_H

// blackbox/FIBBProduct.inl
#ifndef LB_FIBBProduct_INL
#define LB_FIBBProduct_INL
namespace LinBox {

// Blackbox interface
template<class Field> size_t FIBBProduct<Field>:: 
rowdim() const { return Ap->rowdim(); }

template<class Field> size_t FIBBProduct<Field>:: 
coldim() const { return Bp->coldim(); }

template<class Field> const Field& FIBBProduct<Field>:: 
field() const { return Ap->field(); }

template<class Field> typename FIBBProduct<Field>::Matrix& FIBBProduct<Field>:: 
applyRight(typename FIBBProduct<Field>::Matrix& Y, const typename FIBBProduct<Field>::Matrix& X) const
{	Matrix X1(field(), Bp->rowdim(), X.coldim());
	Bp->applyRight(X1, X);
	Ap->applyRight(Y, X1);
	return Y;
}

template<class Field> typename FIBBProduct<Field>::Matrix& FIBBProduct<Field>:: 
applyLeft(typename FIBBProduct<Field>::Matrix& Y, const typename FIBBProduct<Field>::Matrix& X) const
{	Matrix X1(field(), X.rowdim(), Ap->coldim());
	Ap->applyLeft(X1, X);
	Bp->applyLeft(Y, X1);
  	return Y;
}

/* rebind - hom support
*/
// todo: read
template<class Field> std::istream& FIBBProduct<Field>:: 
read(std::istream& is) 
{ return is; }

template<class Field> std::ostream& FIBBProduct<Field>:: 
write(std::ostream& os) const
{	os << std::endl << "%%MatrixMarket matrix composite integer general" << std::endl;
	field().write(os << "% written by LinBox::FIBBProduct< ") << " >" << std::endl;
	Ap->write(os<<std::endl);
	Bp->write(os<<std::endl);
	return os;
}

template<class Field> size_t& FIBBProduct<Field>:: 
rank( size_t& r ) const
{	size_t s, t; return r = std::min(Ap->rank(s), Bp->rank(t)); }

template<class Field> typename FIBBProduct<Field>::Element& FIBBProduct<Field>:: 
det( typename FIBBProduct<Field>::Element& d ) const
{	Ap->det(d); 
	typename Field::Element e; Ap->field().init(e);
	Bp->det(e);
	return Ap->field().mulin(d, e);
}

template<class Field> typename FIBBProduct<Field>::Matrix& FIBBProduct<Field>:: 
solveRight( typename FIBBProduct<Field>::Matrix& Y, const typename FIBBProduct<Field>::Matrix& X ) const
{	Matrix Z(field(), Ap->coldim(), X.coldim());
	Ap->solveRight(Z,X); // A1*Z = X
	return Bp->solveRight(Y,Z); // A2*Y = Z
}

template<class Field> typename FIBBProduct<Field>::Matrix& FIBBProduct<Field>:: 
solveLeft( typename FIBBProduct<Field>::Matrix& Y, const typename FIBBProduct<Field>::Matrix& X ) const
{	Matrix Z(field(), X.rowdim(), Ap->coldim()); 
	Bp->solveLeft(Z,X); // Z*A2 = X
	return Ap->solveLeft(Y,Z); // Y*A1 = Z
}

template<class Field> typename FIBBProduct<Field>::Matrix& FIBBProduct<Field>:: 
nullspaceRandomRight( typename FIBBProduct<Field>::Matrix& N ) const // N: ABN = 0
{	size_t r;
	if (Ap->rowdim() == Ap->coldim() and Ap->rank(r) == Ap->coldim())
		return Bp->nullspaceRandomRight(N);
	else
	{	Matrix N1(N);
		Ap->nullspaceRandomRight(N1);
		return Bp->solveRight(N,N1);
		// a solveRightin would be good if B is a perm.
	}
}

template<class Field> typename FIBBProduct<Field>::Matrix& FIBBProduct<Field>:: 
nullspaceRandomLeft( typename FIBBProduct<Field>::Matrix& N ) const
{	size_t r;
	if (Bp->rowdim() == Bp->coldim() and Bp->rank(r) == Bp->coldim())
		return Ap->nullspaceRandomLeft(N);
	else
	{	Matrix N1(N);
		Bp->nullspaceRandomLeft(N1);
		return Ap->solveLeft(N,N1);
		// a solveLeftin would be good if A is a perm.
	}
}

/*
template<class Field> typename FIBBProduct<Field>::Matrix& FIBBProduct<Field>:: 
genericNullspaceRandomRight( typename FIBBProduct<Field>::Matrix& N ) const
{	Matrix X(field(), rowdim(), N.coldim());
	Matrix R(field(), coldim(), N.coldim());
	R.random();
	applyRight(X, R); // X: X = AR
	solveRight(N, X); // N: AN = X = AR
	return DenseMatrixDomain<Field>(field()).subin(N, R);
}

template<class Field> Matrix& FIBBProduct<Field>:: 
genericNullspaceRandomLeft( Matrix& N ) const
{	Matrix X(field(), N.rowdim(), rowdim());
	Matrix R(field(), N.rowdim(), coldim());
	R.random();
	applyLeft(X, R); // X: X = RA
	solveLeft(N, X); // N: NA = RA
	return BlasMatrixDomain<Field>(field()).subin(N, R);
}
*/

template<class Field> DenseMatrix<Field>& FIBBProduct<Field>:: 
nullspaceBasisRight( DenseMatrix<Field>& N ) const
{	size_t r;
	if (Ap->rowdim() == Ap->coldim() and Ap->rank(r) == Ap->rowdim())
	 	Bp->nullspaceBasisRight(N);
	else 
	{	Matrix N1(field());
		Ap->nullspaceBasisRight(N1);
		N.resize(N1.rowdim(), N1.coldim());
		Bp->solveRight(N, N1);
	}
	return N;
}

template<class Field> DenseMatrix<Field>& FIBBProduct<Field>:: 
nullspaceBasisLeft( DenseMatrix<Field>& N ) const
{	size_t r;
	if (Bp->rowdim() == Bp->coldim() and Bp->rank(r) == Bp->rowdim())
	 	Ap->nullspaceBasisLeft(N);
	else 
	{	Matrix N1(field());
		Bp->nullspaceBasisLeft(N1);
		N.resize(N1.rowdim(), N1.coldim());
		Ap->solveLeft(N, N1);
	}
	return N;
}

/* cstors, dstor */
template<class Field> FIBBProduct<Field>::
FIBBProduct() :Ap(0), Bp(0), allocA(false), allocB(false) {}

template<class Field> FIBBProduct<Field>:: 
FIBBProduct( const FIBB<Field>& A1, const FIBB<Field>& A2 ) 
{ init(A1, A2); } 

template<class Field> FIBBProduct<Field>:: 
FIBBProduct( const FIBB<Field>& A1, const FIBB<Field>& A2, 
			 const FIBB<Field>& A3 ) 
{ init(A1, A2, A3); }

template<class Field> FIBBProduct<Field>:: 
FIBBProduct( const FIBB<Field>& A1, const FIBB<Field>& A2, 
			 const FIBB<Field>& A3, const FIBB<Field>& A4 ) 
{ init(A1, A2, A3, A4); }

template<class Field> FIBBProduct<Field>:: 
FIBBProduct( const FIBB<Field>& A1, const FIBB<Field>& A2, 
			 const FIBB<Field>& A3, const FIBB<Field>& A4, 
			 const FIBB<Field>& A5 ) 
{ init(A1, A2, A3, A4, A5); }

/* initializers */

template<class Field> FIBBProduct<Field>& FIBBProduct<Field>:: 
init( const FIBB<Field>& A1, const FIBB<Field>& A2 )
{ Ap = &A1; Bp = &A2; allocA = allocB = false; return *this; }

template<class Field> FIBBProduct<Field>& FIBBProduct<Field>:: 
init( const FIBB<Field>& A1, const FIBB<Field>& A2, 
	  const FIBB<Field>& A3 )
{ Ap = &A1; 
  allocA = false;
  Bp = new FIBBProduct (A2, A3); 
  allocB = true;
  return *this; 
}

template<class Field> FIBBProduct<Field>& FIBBProduct<Field>:: 
init( const FIBB<Field>& A1, const FIBB<Field>& A2, 
	  const FIBB<Field>& A3, const FIBB<Field>& A4 )
{ 
  Ap = new FIBBProduct (A1, A2); 
  allocA = true;
  Bp = new FIBBProduct (A3, A4); 
  allocB = true;
  return *this; 
}

template<class Field> FIBBProduct<Field>& FIBBProduct<Field>:: 
init( const FIBB<Field>& A1, const FIBB<Field>& A2, 
	  const FIBB<Field>& A3, const FIBB<Field>& A4, 
  	  const FIBB<Field>& A5 )
{ Ap = &A1;
  allocA = false;
  Bp = new FIBBProduct (A1, A2, A3, A4); 
  allocB = true;
  return *this; 
}

}// namespace LinBox

#endif // LB_FIBBProduct_INL
