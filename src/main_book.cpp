// build:console_x64_debug
// Copyright (c) John A. Carlos Jr., all rights reserved.

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <bit>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <set>
#include <map>
#include <queue>
#include <array>
#include <ranges>
#include <numeric>
#include <functional>
#include <random>
using namespace std;
using ssize_t = ptrdiff_t;
#define assert(x_) if (!(x_)) { __debugbreak(); }


// Sequence

template<typename T, typename LessThan> __forceinline void SortInplace(vector<T>& pts, LessThan lessThan) {
	sort(begin(pts), end(pts), lessThan);
}
template<typename T> __forceinline void SortInplace(vector<T>& pts) {
	sort(begin(pts), end(pts));
}
template<typename T, typename LessThan> __forceinline vector<T> Sort(const span<T> pts, LessThan lessThan) {
	vector<T> ptsS(begin(pts), end(pts));
	SortInplace(ptsS, lessThan);
	return ptsS;
}
template<typename T> __forceinline vector<T> Sort(const span<T> pts) {
	vector<T> ptsS(begin(pts), end(pts));
	SortInplace(ptsS);
	return ptsS;
}
template<typename T, typename LessThan> __forceinline bool IsSorted(const span<T> pts, LessThan lessThan) {
	const size_t cPts = pts.size();
	for (size_t i = 1; i < cPts; ++i) {
		if (lessThan(pts[i], pts[i-1])) return false;
	}
	return true;
}
template<typename T> __forceinline bool IsSorted(const span<T> pts) {
	const size_t cPts = pts.size();
	for (size_t i = 1; i < cPts; ++i) {
		if (pts[i] < pts[i-1]) return false;
	}
	return true;
}

template<typename T, typename LessThan> vector<T> ZipperMerge(const span<const T> A, const span<const T> B, LessThan lessThan = std::less{}) {
	vector<T> R;
	R.resize(A.size() + B.size());
	auto a = begin(A);
	const auto a1 = end(A);
	auto b = begin(B);
	const auto b1 = end(B);
	auto r = begin(R);
	const auto r1 = end(R);
	for (;a != a1 && b != b1; ++r) {
		if (!lessThan(*b, *a)) { // a <= b
			*r = *a;
			++a;
		}
		else {
			*r = *b;
			++b;
		}
	}
	for (; a != a1; ++r) {
		*r = *a;
		++a;
	}
	for (; b != b1; ++r) {
		*r = *b;
		++b;
	}
	assert(a == a1);
	assert(b == b1);
	assert(r == r1);
	return R;
}
template<typename T, typename LessThan> void ZipperMergeInplace(vector<T>& A, const span<const T> B, LessThan lessThan = std::less{}) {
	// IDEA: Merge into the end of expanded A, right-to-left.
	const size_t cA = A.size();
	const size_t cB = B.size();
	const size_t cNew = cA + cB;
	A.resize(cNew);
	auto a = rbegin(A) + cB;
	const auto a1 = rend(A);
	auto b = rbegin(B);
	const auto b1 = rend(B);
	auto r = rbegin(A);
	const auto r1 = rend(A);
	for (; a != a1 && b != b1; ++r) {
		if (lessThan(*b, *a)) { // b < a
			*r = *a;
			++a;
		}
		else {
			*r = *b;
			++b;
		}
	}
	for (; b != b1; ++r) {
		*r = *b;
		++b;
	}
	// NOTE: No need to flush the rest of A, since it is already in place.
	assert(b == b1);
}

template<typename T> struct Sequence {
	T* memory = nullptr;
	size_t length = 0;

	Sequence() = default;
	Sequence(Sequence&& o) noexcept = default;
	Sequence(const Sequence& o) = default;
	Sequence<T>& operator=(const Sequence<T>& o) = default;
	Sequence<T>& operator=(Sequence<T>&& o) = default;
	Sequence(T* data_, size_t length_) : memory(data_), length(length_) {}
	Sequence(span<T> s) : memory(s.data()), length(s.size()) {}
	Sequence(const vector<T>& s) : memory((T*)s.data()), length(s.size()) {}

	__forceinline T* data() const { return memory; }
	__forceinline size_t size() const { return length; }
	__forceinline bool empty() const { return !length; }
	__forceinline const T& front() const { assert(length); return memory[0]; }
	__forceinline const T& back() const { assert(length); return memory[length-1]; }
	__forceinline T* begin() const { return memory; }
	__forceinline T* end() const { return memory + length; }
	__forceinline T& operator[](size_t index) { assert(index < length); return memory[index]; }
	__forceinline const bool operator==(const Sequence<T>& o) {
		if (length != o.length) return false;
		for (size_t i = 0; i < length; ++i) {
			if (memory[i] != o.memory[i]) return false;
		}
		return true;
	}
};
// Unsorted sequence
template<typename T> struct SequenceU : public Sequence<T> {};
// Sorted sequence
template<typename T> struct SequenceS : public Sequence<T> {};


template<typename T> bool Contains(SequenceS<T> A, const T& b) {
	return binary_search<T*>(begin(A), end(A), b);
}

template<typename T> bool ContainsAny(SequenceS<T> A, const SequenceS<T> B) {
	if (B.empty()) return false;
	auto a0 = begin(A);
	const auto a1 = end(A);
	auto b = begin(B);
	const auto b1 = end(B);
	auto a = lower_bound(a0, a1, *b);
	while (a != a1 and b != b1) {
		const auto cmp = *a <=> *b;
		if (cmp < 0) ++a;
		else if (cmp > 0) ++b;
		else return true;
	}
	return false;
}
template<typename T> bool ContainsAny(SequenceS<T> A, const SequenceU<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return ContainsAny(A, SequenceS<T>(Bs));
}

template<typename T> bool ContainsAll(SequenceS<T> A, const SequenceS<T> B) {
	if (B.empty()) return true;
	auto a0 = begin(A);
	const auto a1 = end(A);
	auto b = begin(B);
	const auto b1 = end(B);
	auto a = lower_bound(a0, a1, *b);
	while (a != a1 and b != b1) {
		const auto cmp = *a <=> *b;
		if (cmp < 0) ++a;
		else if (cmp > 0) return false;
		else ++b;
	}
	return b == b1;
}
template<typename T> bool ContainsAll(SequenceS<T> A, const SequenceU<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return ContainsAll(A, SequenceS<T>(Bs));
}

template<typename T> void DeduplicateSortedInplace(vector<T>& A) {
	assert(IsSorted<T>(A));
	if (A.size() <= 1) return;
	const size_t cA = A.size();
	size_t iWrite = 0;
	for (size_t iRead = 0; iRead < cA;) {
		const auto& Ar = A[iRead];
		size_t iDup = iRead + 1;
		for (; iDup < cA and Ar == A[iDup]; ++iDup);
		// Keep iRead; eliminate [iRead+1, iDup).
		if (iWrite != iRead) {
			A[iWrite] = A[iRead];
		}
		++iWrite;
		iRead = iDup;
	}
	A.resize(iWrite);
}
template<typename T> void DeduplicateInplace(vector<T>& A) {
	if (!IsSorted<T>(A)) SortInplace(A);
	DeduplicateSortedInplace(A);
}
template<typename T> vector<T> Deduplicate(SequenceS<T> A) {
	vector<T> R { begin(A), end(A) };
	DeduplicateSortedInplace(R);
	return R;
}

template<typename T> void UnionInplace(vector<T>& A, const SequenceS<T> B) {
	if (!IsSorted<T>(A)) SortInplace(A);
	ZipperMergeInplace<T>(A, B, std::less{});
	DeduplicateSortedInplace(A);
}
template<typename T> vector<T> Union(SequenceS<T> A, const SequenceS<T> B) {
	vector<T> R = ZipperMerge<T>(A, B, std::less{});
	DeduplicateSortedInplace(R);
	return R;
}
template<typename T> vector<T> Union(SequenceS<T> A, const SequenceU<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return Union(A, SequenceS<T>(Bs));
}

template<typename T> void IntersectInplace(vector<T>& A, const SequenceS<T> B) {
	if (!IsSorted<T>(A)) SortInplace(A);
	const auto a0 = begin(A);
	auto a = a0;
	const auto a1 = end(A);
	auto b = begin(B);
	const auto b1 = end(B);
	auto write = a0;
	while (a != a1 and b != b1) {
		const auto cmp = *a <=> *b;
		if (cmp < 0) ++a;
		else if (cmp > 0) ++b;
		else {
			if (write != a) {
				*write = *a;
			}
			++a;
			for (; a != a1 and *a == *write; ++a); // Skip duplicates in A.
			++b;
			for (; b != b1 and *b == *write; ++b); // Skip duplicates in B.
			++write;
		}
	}
	const size_t cWritten = write - a0;
	A.resize(cWritten);
}
template<typename T> vector<T> Intersect(SequenceS<T> A, const SequenceS<T> B) {
	// ALTERNATE: Could do the following, but it's less memory efficient for cases where Intersect removes lots of elements.
	// vector<T> R { begin(A), end(A) };
	// IntersectInplace(R, B);
	// return R;
	// Instead, we'll rely on geometric resizing of the result as we accumulate.

	const auto a0 = begin(A);
	auto a = a0;
	const auto a1 = end(A);
	auto b = begin(B);
	const auto b1 = end(B);
	vector<T> R;
	while (a != a1 and b != b1) {
		const auto cmp = *a <=> *b;
		if (cmp < 0) ++a;
		else if (cmp > 0) ++b;
		else {
			T value = *a;
			++a;
			for (; a != a1 and *a == value; ++a); // Skip duplicates in A.
			++b;
			for (; b != b1 and *b == value; ++b); // Skip duplicates in B.
			R.emplace_back(move(value));
		}
	}
	return R;
}
template<typename T> vector<T> Intersect(SequenceS<T> A, const SequenceU<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return Intersect(A, SequenceS<T>(Bs));
}

template<typename T> void SubtractInplace(vector<T>& A, const SequenceS<T> B) {
	if (!IsSorted<T>(A)) SortInplace(A);
	const auto a0 = begin(A);
	auto a = a0;
	const auto a1 = end(A);
	auto b = begin(B);
	const auto b1 = end(B);
	auto write = a0;
	while (a != a1 and b != b1) {
		const auto cmp = *a <=> *b;
		if (cmp < 0) {
			if (write != a) {
				*write = *a;
			}
			++a;
			++write;
		}
		else if (cmp > 0) {
			++b;
		}
		else {
			++a;
			for (; a != a1 and *a == *write; ++a); // Skip duplicates in A.
			++b;
			for (; b != b1 and *b == *write; ++b); // Skip duplicates in B.
		}
	}
	if (a != a1) {
		// Flush remaining A elements.
		copy(a, a1, write);
		write += (a1 - a);
	}
	const size_t cWritten = write - a0;
	A.resize(cWritten);
}
template<typename T> vector<T> Subtract(SequenceS<T> A, const SequenceS<T> B) {
	vector<T> R { begin(A), end(A) };
	SubtractInplace(R, B);
	return R;
}
template<typename T> vector<T> Subtract(SequenceS<T> A, const SequenceU<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return Subtract(A, SequenceS<T>(Bs));
}

// Symmetric difference
//   SymmetricDifference(A,B) = Subtract(Union(A,B), Intersection(A,B))
template<typename T> vector<T> SymmetricDifference(SequenceS<T> A, const SequenceS<T> B) {
	vector<T> U = Union(A, B);
	vector<T> I = Intersect(A, B);
	SubtractInplace(U, SequenceS<T>(I));
	return U;
}
template<typename T> void SymmetricDifferenceInplace(vector<T>& A, const SequenceS<T> B) {
	// IDEA: Union(A, B) - Intersect(A, B)
	vector<T> I = Intersect(SequenceS<T>(A), B);
	UnionInplace(A, B);
	SubtractInplace(A, SequenceS<T>(I));
}

template<typename T> bool Equal(const SequenceS<T>& A, const SequenceS<T>& B) {
	if (A.size() != B.size()) return false;
	const auto a = begin(A);
	const auto a1 = end(A);
	const auto b = begin(B);
	const auto b1 = end(B);
	return equal(a, a1, b, b1);
}


// ============================================================================
// Unsorted sequence

template<typename T> bool Contains(SequenceU<T> A, const T& b) {
	for (const auto& a : A) {
		if (a == b) return true;
	}
	return false;
}

template<typename T> bool ContainsAny(SequenceU<T> A, const SequenceS<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	return ContainsAny(SequenceS<T>(As), B);
}
template<typename T> bool ContainsAny(SequenceU<T> A, const SequenceU<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return ContainsAny(SequenceS<T>(As), SequenceS<T>(Bs));
}

template<typename T> bool ContainsAll(SequenceU<T> A, const SequenceS<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	return ContainsAll(SequenceS<T>(As), B);
}
template<typename T> bool ContainsAll(SequenceU<T> A, const SequenceU<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return ContainsAll(SequenceS<T>(As), SequenceS<T>(Bs));
}

template<typename T> vector<T> DeduplicateSort(SequenceU<T> A) {
	vector<T> R { begin(A), end(A) };
	SortInplace(R);
	DeduplicateSortedInplace(R);
	return R;
}
template<typename T> vector<T> Deduplicate(SequenceU<T> A) {
	// ALTERNATE: Could do the same thing with DeduplicateNonOrderPreserving to make a sorted set, and then Contains(SequenceS<T>, T) loop.
	unordered_set<T> D { begin(A), end(A) };
	vector<T> R;
	for (const auto& a : A) {
		if (D.contains(a)) R.push_back(a);
	}
	return R;
}

template<typename T> vector<T> Union(SequenceU<T> A, const SequenceS<T> B) {
	return Union(B, A);
}
template<typename T> vector<T> Union(SequenceU<T> A, const SequenceU<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	return Union(As, B);
}

template<typename T> vector<T> Intersect(SequenceU<T> A, const SequenceS<T> B) {
	return Intersect(B, A);
}
template<typename T> vector<T> Intersect(SequenceU<T> A, const SequenceU<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	return Intersect(SequenceS<T>(As), B);
}

template<typename T> vector<T> Subtract(SequenceU<T> A, const SequenceS<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	return Subtract(SequenceS<T>(As), B);
}
template<typename T> vector<T> Subtract(SequenceU<T> A, const SequenceU<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs{ begin(B), end(B) };
	SortInplace(Bs);
	return Subtract(SequenceS<T>(As), SequenceS<T>(Bs));
}

template<typename T> vector<T> SymmetricDifference(SequenceU<T> A, const SequenceS<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	return SymmetricDifference(SequenceS<T>(As), B);
}
template<typename T> vector<T> SymmetricDifference(SequenceU<T> A, const SequenceU<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs{ begin(B), end(B) };
	SortInplace(Bs);
	return SymmetricDifference(SequenceS<T>(As), SequenceS<T>(Bs));
}

template<typename T> bool Equal(const SequenceU<T>& A, const SequenceU<T>& B) {
	if (A.size() != B.size()) return false;
	const auto a = begin(A);
	const auto a1 = end(A);
	const auto b = begin(B);
	const auto b1 = end(B);
	return equal(a, a1, b, b1);
}


struct NaiveSet {
	set<int> points;

	NaiveSet() = default;
	explicit NaiveSet(const span<int> points_) {
		for (const auto& p : points_) {
			points.insert(p);
		}
	}
};
bool Contains(const NaiveSet& A, int pt) {
	return A.points.contains(pt);
}
bool ContainsAny(const NaiveSet& A, const span<int> pts) {
	for (const auto& p : pts) {
		if (Contains(A, p)) return true;
	}
	return false;
}
bool ContainsAll(const NaiveSet& A, const span<int> pts) {
	for (const auto& p : pts) {
		if (!Contains(A, p)) return false;
	}
	return true;
}
NaiveSet Intersect(const NaiveSet& A, const NaiveSet& B) {
	NaiveSet result;
	for (const auto& p : A.points) {
		if (B.points.contains(p)) {
			result.points.insert(p);
		}
	}
	return result;
}
NaiveSet Subtract(const NaiveSet& A, const NaiveSet& B) {
	NaiveSet result;
	for (const auto& p : A.points) {
		if (!B.points.contains(p)) {
			result.points.insert(p);
		}
	}
	return result;
}
NaiveSet SymmetricDifference(const NaiveSet& A, const NaiveSet& B) {
	// IDEA: Subtract(A, B) + Subtract(B, A)
	NaiveSet result = Subtract(A, B);
	NaiveSet temp = Subtract(B, A);
	for (const auto& p : temp.points) {
		result.points.insert(p);
	}
	return result;
}
NaiveSet Union(const NaiveSet& A, const NaiveSet& B) {
	NaiveSet result;
	for (const auto& p : A.points) {
		result.points.insert(p);
	}
	for (const auto& p : B.points) {
		result.points.insert(p);
	}
	return result;
}
bool Equal(const NaiveSet& A, const NaiveSet& B) {
	if (A.points.size() != B.points.size()) return false;
	for (const auto& p : A.points) {
		if (!B.points.contains(p)) return false;
	}
	return true;
}
bool Equal(const NaiveSet& A, const span<int> B) {
	NaiveSet Bs{ B };
	return Equal(A, Bs);
}
bool Equal(const span<int> A, const NaiveSet& B) {
	NaiveSet As{ A };
	return Equal(As, B);
}
void TestSequences() {
	minstd_rand rng(0x1234);
	geometric_distribution<int> distGeo(0.01);
	uniform_int_distribution<int> distInt(1, 100);
	auto randomInts = [&]() -> vector<int> {
		const auto cElements = 1 + distGeo(rng);
		vector<int> pts(cElements);
		for (auto& p : pts) {
			p = distInt(rng);
		}
		return pts;
		};

	vector<int> S;
	NaiveSet R;
	const function<void()> fns[] = {
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			for (const auto& p : pts) {
				const auto Rc = Contains(R, p);
				const auto Sc = Contains<int>(SequenceS<int>(S), p);
				assert(Rc == Sc);
			}
		},
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			const auto Rc = ContainsAny(R, pts);
			const auto Sc = ContainsAny<int>(SequenceS<int>(S), SequenceU<int>(pts));
			assert(Rc == Sc);
		},
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			const auto Rc = ContainsAll(R, pts);
			const auto Sc = ContainsAll<int>(SequenceS<int>(S), SequenceU<int>(pts));
			assert(Rc == Sc);
		},
		[&]() {
			// Intersect
			assert(Equal(S, R));
			vector<int> SI = Intersect(SequenceS<int>(S), SequenceS<int>(S));
			assert(Equal(SequenceS<int>(SI), SequenceS<int>(S)));
			NaiveSet RI = Intersect(R, R);
			assert(Equal(RI, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(SequenceU<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Intersect(SequenceS<int>(S), SequenceS<int>(Sr));
			auto Rn = Intersect(R, Rr);
			assert(Equal(Sn, Rn));
			IntersectInplace(S, SequenceS<int>(Sr));
			assert(Equal(SequenceS<int>(S), SequenceS<int>(Sn)));
			R = Rn;
		},
		[&]() {
			// Subtract
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(SequenceU<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Subtract<int>(SequenceS<int>(S), SequenceS<int>(Sr));
			auto Rn = Subtract(R, Rr);
			assert(Equal(Sn, Rn));
			SubtractInplace(S, SequenceS<int>(Sr));
			assert(Equal(SequenceS<int>(S), SequenceS<int>(Sn)));
			R = Rn;
		},
		[&]() {
			// Union
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(SequenceU<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Union<int>(SequenceS<int>(S), SequenceS<int>(Sr));
			auto Rn = Union(R, Rr);
			assert(Equal(Sn, Rn));
			UnionInplace<int>(S, SequenceS<int>(Sr));
			assert(Equal(SequenceS<int>(S), SequenceS<int>(Sn)));
			R = Rn;
		},
		[&]() {
			// SymmetricDifference
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(SequenceU<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = SymmetricDifference(SequenceS<int>(S), SequenceS<int>(Sr));
			auto Rn = SymmetricDifference(R, Rr);
			assert(Equal(Sn, Rn));
			SymmetricDifferenceInplace<int>(S, SequenceS<int>(Sr));
			assert(Equal(SequenceS<int>(S), SequenceS<int>(Sn)));
			R = Rn;
		},
	};
	for (size_t i = 0; i < 1000000; ++i) {
		const size_t idx = distInt(rng) % size(fns);
		if (i == 14180000) __debugbreak();
		fns[idx]();
	}
}


// ============================================================================
// Intervals

template<typename T> struct IntervalII { T p0, p1; };
template<typename T> __forceinline bool operator==(const IntervalII<T>& A, const IntervalII<T>& B) {
	return A.p0 == B.p0 and A.p1 == B.p1;
}


template<typename T> struct LessThanStartIntervalII {
	__forceinline bool operator()(const IntervalII<T>& A, const IntervalII<T>& B) const {
		return A.p0 < B.p0;
	}
};

	template<typename T> __forceinline bool Overlaps(const IntervalII<T>& A, const IntervalII<T>& B) {
		// Assume A.p0 <= A.p1, and the same for B.
		// There's no overlap when A.p1 < B.p0 (A entirely left of B), or A.p0 > B.p1 (A entirely right of B).
		// Negation gives us the final equation.
		return !(A.p1 < B.p0 or A.p0 > B.p1);
	}

// Interval contains given point.
template<typename T> __forceinline bool Contains(const IntervalII<T>& A, const T& p) {
	return A.p0 <= p and p <= A.p1;
}
// Interval contains any of the given points.
template<typename T> __forceinline bool ContainsAny(const IntervalII<T>& A, const span<T> pts) {
	for (const auto& p : pts) {
		if (Contains(A, p)) return true;
	}
	return false;
}
// Interval contains all of the given points.
template<typename T> __forceinline bool ContainsAll(const IntervalII<T>& A, const span<T> pts) {
	for (const auto& p : pts) {
		if (!Contains(A, p)) return false;
	}
	return true;
}


template<typename T> __forceinline bool Adjacent(const IntervalII<T>& A, const IntervalII<T>& B) {
	return A.p1 + 1 == B.p0 or B.p1 + 1 == A.p0;
}
template<typename T> __forceinline bool AdjacentOrOverlap(const IntervalII<T>& A, const IntervalII<T>& B) {
	return Adjacent(A, B) or Overlaps(A, B);
}

template<typename T> vector<IntervalII<T>> Merge(const span<IntervalII<T>> intervals) {
	auto MergeSorted = [](const span<const IntervalII<T>> intervals) {
		vector<IntervalII<T>> result;
		if (!intervals.empty()) {
			result.push_back(intervals[0]);
			const size_t cIntervals = intervals.size();
			for (size_t i = 1; i < cIntervals; ++i) {
				auto& last = result.back();
				const auto& curr = intervals[i];
				if (AdjacentOrOverlap(last, curr)) {
					last.p1 = max(last.p1, curr.p1);
				}
				else {
					result.push_back(curr);
				}
			}
		}
		return result;
	};
	if (IsSorted(intervals, LessThanStartIntervalII<T>{})) {
		return MergeSorted(intervals);
	}
	else {
		vector<IntervalII<T>> sorted = Sort(intervals, LessThanStartIntervalII<T>{});
		return MergeSorted(sorted);
	}
}
template<typename T> void MergeSortedInplace(vector<IntervalII<T>>& intervals) {
	if (intervals.empty()) return;
	const size_t cIntervals = intervals.size();
	size_t iWrite = 0;
	for (size_t i = 1; i < cIntervals; ++i) {
		auto& last = intervals[iWrite];
		const auto& curr = intervals[i];
		if (AdjacentOrOverlap(last, curr)) {
			// Merge with the last interval.
			last.p1 = max(last.p1, curr.p1);
		}
		else {
			// Left shift write the current interval.
			++iWrite;
			if (iWrite != i) {
				intervals[iWrite] = curr;
			}
		}
	}
	// Account for left-shifting.
	intervals.resize(iWrite + 1);
}
template<typename T> void MergeInplace(vector<IntervalII<T>>& intervals) {
	SortInplace(intervals, LessThanStartIntervalII<T>{});
	MergeSortedInplace(intervals);
}

template<typename T> vector<IntervalII<T>> SortedIntervalsFromPoints(const span<T> points) {
	// IDEA: Sort the points, then create intervals from adjacent points.
	if (points.empty()) return {};
	auto sorted = Sort(points);
	vector<IntervalII<T>> intervals;
	intervals.push_back(IntervalII<T>{sorted[0], sorted[0]});
	const size_t cPoints = sorted.size();
	for (size_t i = 1; i < cPoints; ++i) {
		auto& last = intervals.back();
		if (last.p1 + 1 == sorted[i]) {
			// Adjacent to the last interval.
			last.p1 = sorted[i];
		}
		else if (last.p1 < sorted[i]) {
			// Not adjacent, create a new interval.
			intervals.push_back(IntervalII<T>{sorted[i], sorted[i]});
		}
	}
	return intervals;
}

template<typename T> struct IntervalSet {
	// Intervals are non-overlapping, non-adjacent, and sorted.
	vector<IntervalII<T>> intervals;

	IntervalSet() = default;
	explicit IntervalSet(const span<IntervalII<T>> intervals_) {
		intervals = Merge(intervals_);
	}
	explicit IntervalSet(const IntervalII<T>& interval_) {
		const span<IntervalII<T>> intervals_{ (IntervalII<T>*)&interval_, 1 };
		intervals = Merge(intervals_);
	}
	explicit IntervalSet(const span<T> points) {
		intervals = SortedIntervalsFromPoints(points);
	}
	~IntervalSet() noexcept = default;
};

template<typename T> void UnionInplace(IntervalSet<T>& A, const IntervalII<T>& interval) {
	// IDEA: Binary search for interval.p0 and again for interval.p1, giving us the adjacent/overlapping intervals to merge together.
	// Then collapse that subsequence of intervals down to one interval.
	auto& intervals = A.intervals;
	if (intervals.empty()) {
		intervals.push_back(interval);
	}
	else {
		// L = first interval with p1 >= interval.p0 - 1
		auto L = lower_bound(begin(intervals), end(intervals), interval.p0, [](const IntervalII<T>& a, const T& b) {
			return a.p1 + 1 < b;
		});
		const size_t iL = L - begin(intervals);
		// R = last interval with p0 <= interval.p1 + 1
		auto Rr = lower_bound(rbegin(intervals), rend(intervals), interval.p1, [](const IntervalII<T>& a, const T& b) {
			return a.p0 > b + 1;
		});
		const size_t iR = intervals.size() - (Rr - rbegin(intervals));
		auto R = begin(intervals) + iR;
		if (L != R) {
			--R;
			if (L == R) {
				// Merge with a single interval.
				L->p0 = min(L->p0, interval.p0);
				L->p1 = max(L->p1, interval.p1);
			}
			else {
				// Merge with multiple intervals.
				L->p0 = min(L->p0, interval.p0);
				L->p1 = max(R->p1, interval.p1);
				intervals.erase(L + 1, R + 1);
			}
		}
		else {
			// it = first interval with p1 >= interval.p0
			auto it = lower_bound(begin(intervals), end(intervals), interval.p0, [](const IntervalII<T>& a, const T& b) {
				return a.p1 < b;
			});
			intervals.insert(it, interval);
		}
	}
}
template<typename T> void UnionSortedInplace(IntervalSet<T>& A, const span<const IntervalII<T>> toInsert) {
	if (toInsert.empty()) return;
	ZipperMergeInplace<IntervalII<T>>(A.intervals, toInsert, LessThanStartIntervalII<T>{});
	MergeSortedInplace(A.intervals);
};
template<typename T> void UnionInplace(IntervalSet<T>& A, const span<const IntervalII<T>> toInsert) {
	if (IsSorted(toInsert, LessThanStartIntervalII<T>{})) {
		UnionSortedInplace(A, toInsert);
	}
	else {
		UnionSortedInplace(A, Sort(toInsert, LessThanStartIntervalII<T>{}));
	}
}
template<typename T> void UnionInplace(IntervalSet<T>& A, const IntervalSet<T>& B) {
	UnionSortedInplace<T>(A, B.intervals);
}
template<typename T> IntervalSet<T> Union(const IntervalSet<T>& A, const IntervalSet<T>& B) {
	// IDEA: Zipper merge the two sorted interval sets, then merge adjacents/overlaps.
	// This is O(|A| + |B|)
	IntervalSet<T> result;
	result.intervals = ZipperMerge<IntervalII<T>>(A.intervals, B.intervals, LessThanStartIntervalII<T>{});
	MergeSortedInplace(result.intervals);
	return result;
}

template<typename T> bool Contains(const IntervalSet<T>& A, const T& pt) {
	// Last interval with: interval.p0 <= pt
	const auto a0 = begin(A.intervals);
	const auto a1 = end(A.intervals);
	auto it = lower_bound(a0, a1, pt, [](const IntervalII<T>& a, const T& b) {
		return a.p0 <= b;
	});
	if (it == a0) return false;
	--it;
	// it : last interval with: pt <= interval.p1
	return Contains(*it, pt);
}
template<typename T> bool ContainsAnySorted(const IntervalSet<T>& A, const span<T> pointsSorted) {
	// Idea: If we sort the given points, we can do a single pass comparison.
	// This is O(|I| + |P| log |P|) when P is unsorted; O(|I| + |P|) when P is sorted.
	auto p = begin(pointsSorted);
	const auto p1 = end(pointsSorted);
	auto interval = begin(A.intervals);
	const auto interval1 = end(A.intervals);
	while (p != p1 and interval != interval1) {
		if (*p < interval->p0) {
			++p; // Point is before the current interval, move to next point.
		}
		else if (*p > interval->p1) {
			++interval; // Point is after the current interval, move to next interval.
		}
		else {
			return true; // Point is within the current interval.
		}
	}
	return false;
}
template<typename T> bool ContainsAny(const IntervalSet<T>& A, const span<T> pts) {
	// Alternative: for each given point, binary search the intervals. This would be O(|P| log |I|).
	if (IsSorted<T>(pts)) {
		return ContainsAnySorted<T>(A, pts);
	}
	else {
		vector<T> sorted = Sort<T>(pts);
		return ContainsAnySorted<T>(A, sorted);
	}
}
template<typename T> bool ContainsAllSorted(const IntervalSet<T>& A, const span<T> pointsSorted) {
	assert(IsSorted<T>(pointsSorted));
	// Idea: If we sort the given points, we can do a single pass comparison.
	// This is O(|I| + |P| log |P|) when P is unsorted; O(|I| + |P|) when P is sorted.
	auto p = begin(pointsSorted);
	const auto p1 = end(pointsSorted);
	auto interval = begin(A.intervals);
	const auto interval1 = end(A.intervals);
	while (p != p1 and interval != interval1) {
		if (*p < interval->p0) {
			return false; // Point is before the current interval.
		}
		else if (*p > interval->p1) {
			++interval; // Point is after the current interval, move to next interval.
		}
		else {
			// Point is within the current interval.
			++p;
		}
	}
	return p == p1;
}
template<typename T> bool ContainsAll(const IntervalSet<T>& A, const span<T> pts) {
	// Alternative: for each given point, binary search the intervals. This would be O(|P| log |I|).
	if (IsSorted<T>(pts)) {
		return ContainsAllSorted<T>(A, pts);
	}
	else {
		vector<T> sorted = Sort<T>(pts);
		return ContainsAllSorted<T>(A, sorted);
	}
}

template<typename T> optional<IntervalII<T>> Intersect(const span<const IntervalII<T>> intervals) {
	// IDEA: Accumulate max of p0, min of p1. If they all overlap, this gets us the intersection.
	if (!intervals.empty()) {
		T p0 = intervals[0].p0;
		T p1 = intervals[0].p1;
		const size_t cIntervals = intervals.size();
		for (size_t i = 1; i < cIntervals; ++i) {
			p0 = max(p0, intervals[i].p0);
			p1 = min(p1, intervals[i].p1);
		}
		if (p0 <= p1) {
			return IntervalII<T>{p0, p1};
		}
	}
	return nullopt;
}
template<typename T> IntervalSet<T> Intersect(const IntervalSet<T>& A, const IntervalSet<T>& B) {
	// IDEA: 2-iterator walk until there's an overlap; emit overlap intersection and advance past the earlier interval.
	// This is O(|A| + |B|)
	auto a = begin(A.intervals);
	auto b = begin(B.intervals);
	const auto a1 = end(A.intervals);
	const auto b1 = end(B.intervals);
	IntervalSet<T> result;
	while (a != a1 and b != b1) {
		if (a->p1 < b->p0) {
			// A is before B, move to next A.
			++a;
		}
		else if (b->p1 < a->p0) {
			// B is before A, move to next B.
			++b;
		}
		else {
			// Overlapping intervals, add intersection.
			result.intervals.push_back(IntervalII<T>{max(a->p0, b->p0), min(a->p1, b->p1)});
			const auto cmp = a->p1 <=> b->p1;
			if (cmp <= 0) ++a; // Move to next A.
			if (cmp >= 0) ++b; // Move to next B.
		}
	}
	// NOTE: No need to mergeSortedInplace; no possibility of new adjacencies with intersection.
	return result;
}
template<typename T> void IntersectInplace(IntervalSet<T>& A, const IntervalSet<T>& B) {
	// IDEA: Expand A to maximum size, run the same Intersect algorithm but in reverse order writing to end of A,
	// and then shift left to the start, and resize A.
	const size_t cA = A.intervals.size();
	const size_t cB = B.intervals.size();
	// E.g. cA=3, cB=2, max interleaving, we can end up with 6 intervals.
	const size_t cMax = 1 + cA + cB;
	A.intervals.resize(cMax);
	auto a = rbegin(A.intervals) + (cMax - cA);
	auto b = rbegin(B.intervals);
	const auto a1 = rend(A.intervals);
	const auto b1 = rend(B.intervals);
	auto write = rbegin(A.intervals);
	while (a != a1 and b != b1) {
		if (a->p0 > b->p1) {
			// A is after B, move to next A.
			++a;
		}
		else if (b->p0 > a->p1) {
			// B is after A, move to next B.
			++b;
		}
		else {
			// Overlapping intervals, add intersection.
			const auto pmax = max(a->p0, b->p0);
			write->p0 = pmax;
			write->p1 = min(a->p1, b->p1);
			++write;
			const auto cmp = a->p0 <=> b->p0;
			if (cmp >= 0) ++a; // Move to next A.
			if (cmp <= 0) ++b; // Move to next B.
		}
	}
	const size_t cWritten = write - rbegin(A.intervals);
	const size_t cShift = cMax - cWritten;
	if (cShift) {
		copy(begin(A.intervals) + cShift, end(A.intervals), begin(A.intervals));
	}
	A.intervals.resize(cWritten);
}


template<typename T> void SubtractPush(vector<IntervalII<T>>& intervals, const IntervalII<T>& A, const IntervalII<T>& B) {
	if (!Overlaps(A, B)) {
		intervals.push_back(A); // No overlap, return A as is.
	}
	else {
		if (A.p0 < B.p0) {
			// Left part remains.
			intervals.push_back(IntervalII<T>{A.p0, B.p0 - 1});
		}
		if (A.p1 > B.p1) {
			// Right part remains.
			intervals.push_back(IntervalII<T>{B.p1 + 1, A.p1});
		}
	}
};
template<typename T> IntervalSet<T> Subtract(const IntervalII<T>& A, const IntervalII<T>& B) {
	IntervalSet<T> result;
	SubtractPush(result.intervals, A, B);
	return result;
}
template<typename T> void SubtractInplace(IntervalSet<T>& A, const IntervalII<T>& interval) {
	// IDEA: Binary search for interval.p0 and again for interval.p1, giving us the interval subsequence to consider.
	// If the subsequence has length > 2, we can trivially drop the interior ones and do interval minus interval on the first and last.
	auto& intervals = A.intervals;
	if (intervals.empty()) return;

	// L = first interval that ends >= given interval.p0
	auto L = lower_bound(begin(intervals), end(intervals), interval.p0, [](const IntervalII<T>& a, const T& b) {
		return a.p1 < b;
	});
	if (L == end(intervals)) return;
	const size_t iL = L - begin(intervals);
	// R = last interval that starts <= given interval.p1
	auto Rr = lower_bound(rbegin(intervals), rend(intervals), interval.p1, [](const IntervalII<T>& a, const T& b) {
		return a.p0 > b;
	});
	const size_t iR = intervals.size() - 1 - (Rr - rbegin(intervals));
	auto R = begin(intervals) + iR;

	vector<IntervalII<T>> temp;
	temp.reserve(4);
	SubtractPush(temp, *L, interval);
	if (L != R) SubtractPush(temp, *R, interval);
	const size_t cRemove = R - L + 1;
	const size_t cInsert = temp.size();
	const size_t cIntervals = intervals.size();
	const size_t cIntervalsNew = cIntervals - cRemove + cInsert;
	if (cIntervals != cIntervalsNew) {
		intervals.resize(cIntervalsNew);
		const size_t iMoveSrc = iR + 1;
		const size_t iMoveDst = iMoveSrc + cInsert - cRemove;
		const size_t cMove = cIntervals - iMoveSrc;
		if (iMoveDst > iMoveSrc)
			copy_backward(begin(intervals) + iMoveSrc, begin(intervals) + iMoveSrc + cMove, begin(intervals) + iMoveDst);
		else if (iMoveDst < iMoveSrc)
			copy(begin(intervals) + iMoveSrc, begin(intervals) + iMoveSrc + cMove, begin(intervals) + iMoveDst);
	}
	const size_t iRewrite = iL;
	copy(begin(temp), end(temp), begin(intervals) + iRewrite);
}
template<typename T> IntervalSet<T> Subtract(const IntervalSet<T>& A, const IntervalSet<T>& B) {
	IntervalSet<T> result;
	auto& intervals = result.intervals;
	auto a = begin(A.intervals);
	auto b = begin(B.intervals);
	const auto a1 = end(A.intervals);
	const auto b1 = end(B.intervals);
	// As we iterate, we may break the `a` interval into two parts: left and right.
	// The left part gets pushed to the result. The right part gets stored here, as a prefix to the list of intervals to go over.
	// Traditionally this would be a stack/queue, but we can optimally just use one optional slot.
	optional<IntervalII<T>> aAlt;
	while ((aAlt.has_value() || a != a1) and b != b1) {
		const IntervalII<T> ai = aAlt.has_value() ? *aAlt : *a;
		if (ai.p1 < b->p0) {
			// A is before B, add A to result and move to next A.
			intervals.push_back(ai);
			if (aAlt.has_value()) aAlt.reset();
			else ++a;
		}
		else if (b->p1 < ai.p0) {
			// B is before A, move to next B.
			++b;
		}
		else {
			// Overlapping intervals, subtract B from A.
			if (ai.p0 < b->p0) {
				// Left part remains.
				intervals.push_back(IntervalII<T>{ai.p0, b->p0 - 1});
			}
			if (ai.p1 > b->p1) {
				// Right part remains.
				if (!aAlt.has_value()) ++a;
				aAlt = IntervalII<T> { b->p1 + 1, ai.p1 };
				++b;
			}
			else {
				if (aAlt.has_value()) aAlt.reset();
				else ++a;
			}
		}
	}
	if (aAlt.has_value()) {
		intervals.push_back(*aAlt);
	}
	while (a != a1) {
		// Add the rest of A.
		intervals.push_back(*a);
		++a;
	}
	return result;
}
template<typename T> void SubtractInplace(IntervalSet<T>& A, const IntervalSet<T>& B) {
	// NOTE: Since subtraction may arbitrarily increase or decrease the  number of intervals, there's no static scheme
	// for in-place rewriting. It would have to be dynamic, i.e. shuffle around a suffix of intervals on demand, when we run
	// into memory overlap situations.
	// This would indeed get us improvements in the case of the number of intervals decreasing, but likely not much faster
	// in the case of the number of intervals increasing.
	// So for now I'll just redirect to non-inplace algorithm.
	A = Subtract(A, B);
}

// Symmetric difference
//   SymmetricDifference(A,B) = Subtract(Union(A,B), Intersection(A,B))
template<typename T> IntervalSet<T> SymmetricDifference(const IntervalSet<T>& A, const IntervalSet<T>& B) {
	IntervalSet<T> U = Union(A, B);
	IntervalSet<T> I = Intersect(A, B);
	SubtractInplace(U, I);
	return U;
}
template<typename T> void SymmetricDifferenceInplace(IntervalSet<T>& A, const IntervalSet<T>& B) {
	// IDEA: Union(A, B) - Intersect(A, B)
	IntervalSet<T> I = Intersect(A, B);
	UnionInplace(A, B);
	SubtractInplace(A, I);
}

// Negation (complement) with a given domain.
// Effectively, Subtract({domain}, A).
template<typename T> IntervalSet<T> Complement(const IntervalSet<T>& A, const IntervalII<T>& domain) {
	// IDEA: Subtract domain from A.
	IntervalSet<T> result { domain };
	if (!A.intervals.empty()) {
		SubtractInplace(result, A);
	}
	return result;
}

template<typename T> bool Equal(const IntervalSet<T>& A, const IntervalSet<T>& B) {
	// IDEA: Compare the intervals.
	if (A.intervals.size() != B.intervals.size()) return false;
	const auto a = begin(A.intervals);
	const auto a1 = end(A.intervals);
	const auto b = begin(B.intervals);
	const auto b1 = end(B.intervals);
	return equal(a, a1, b, b1);
}



NaiveSet Complement(const NaiveSet& A, const IntervalII<int>& domain) {
	NaiveSet result;
	for (int i = domain.p0; i <= domain.p1; ++i) {
		if (!A.points.contains(i)) {
			result.points.insert(i);
		}
	}
	return result;
}
bool Equal(const NaiveSet& A, const IntervalSet<int>& B) {
	vector<int> points { begin(A.points), end(A.points) };
	IntervalSet<int> C { points };
	return Equal(C, B);
}
bool Equal(const IntervalSet<int>& A, const NaiveSet& B) {
	return Equal(B, A);
}


void TestIntervalSet() {
	assert(Adjacent(IntervalII<int>{1, 2}, IntervalII<int>{3, 4}));
	assert(!Adjacent(IntervalII<int>{1, 2}, IntervalII<int>{4, 4}));
	assert(AdjacentOrOverlap(IntervalII<int>{1, 2}, IntervalII<int>{3, 4}));
	assert(AdjacentOrOverlap(IntervalII<int>{1, 2}, IntervalII<int>{2, 3}));
	assert(!AdjacentOrOverlap(IntervalII<int>{1, 2}, IntervalII<int>{4, 4}));
	assert(Contains(IntervalII<int>{1, 2}, 1));
	assert(!Contains(IntervalII<int>{1, 2}, 3));
	array<int, 2> pts23 { 2, 3 };
	assert(ContainsAny<int>(IntervalII<int>{1, 2}, pts23));
	array<int, 2> pts34 { 3, 4 };
	assert(!ContainsAny<int>(IntervalII<int>{1, 2}, pts34));
	assert(ContainsAll<int>(IntervalII<int>{1, 3}, pts23));
	assert(!ContainsAll<int>(IntervalII<int>{1, 3}, pts34));
	array<IntervalII<int>, 2> intervals2334 { IntervalII<int>{2, 3}, IntervalII<int>{3, 4} };
	optional<IntervalII<int>> intersection = Intersect<int>(intervals2334);
	const IntervalII<int> expectedIntersection { 3, 3 };
	assert(intersection.value() == expectedIntersection);

	minstd_rand rng(0x1234);
	geometric_distribution<int> distGeo(0.01);
	uniform_int_distribution<int> distInt(1, 100);
	auto randomInts = [&]() -> vector<int> {
		const auto cElements = 1 + distGeo(rng);
		vector<int> pts(cElements);
		for (auto& p : pts) {
			p = distInt(rng);
		}
		return pts;
	};

	IntervalSet<int> S;
	NaiveSet R;
	const function<void()> fns[] = {
		[&]() {
			assert(Equal(S, R));
			const auto a = distInt(rng);
			const auto b = distInt(rng);
			const IntervalII<int> interval { min(a, b), max(a, b) };
			const auto Rn = Complement(R, interval);
			const auto Sn = Complement<int>(S, interval);
			assert(Equal(Sn, Rn));
			R = Rn;
			S = Sn;
		},
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			for (const auto& p : pts) {
				const auto Rc = Contains(R, p);
				const auto Sc = Contains<int>(S, p);
				assert(Rc == Sc);
			}
		},
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			const auto Rc = ContainsAny(R, pts);
			const auto Sc = ContainsAny<int>(S, pts);
			assert(Rc == Sc);
		},
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			const auto Rc = ContainsAll(R, pts);
			const auto Sc = ContainsAll<int>(S, pts);
			assert(Rc == Sc);
		},
		[&]() {
			// Intersect
			assert(Equal(S, R));
			IntervalSet<int> SI = Intersect(S, S);
			assert(Equal(SI, S));
			NaiveSet RI = Intersect(R, R);
			assert(Equal(RI, R));
			vector<int> pts = randomInts();
			IntervalSet<int> Sr { pts };
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Intersect(S, Sr);
			auto Rn = Intersect(R, Rr);
			assert(Equal(Sn, Rn));
			IntersectInplace(S, Sr);
			assert(Equal(S, Sn));
			R = Rn;
		},
		[&]() {
			// Subtract
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			IntervalSet<int> Sr { pts };
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Subtract<int>(S, Sr);
			auto Rn = Subtract(R, Rr);
			assert(Equal(Sn, Rn));
			SubtractInplace(S, Sr);
			assert(Equal(S, Sn));
			R = Rn;
		},
		[&]() {
			// Union
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			IntervalSet<int> Sr { pts };
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Union<int>(S, Sr);
			auto Rn = Union(R, Rr);
			assert(Equal(Sn, Rn));
			UnionInplace<int>(S, Sr);
			assert(Equal(S, Sn));
			R = Rn;
		},
		[&]() {
			// SymmetricDifference
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			IntervalSet<int> Sr { pts };
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = SymmetricDifference(S, Sr);
			auto Rn = SymmetricDifference(R, Rr);
			assert(Equal(Sn, Rn));
			SymmetricDifferenceInplace<int>(S, Sr);
			assert(Equal(S, Sn));
			R = Rn;
		},
	};
	for (size_t i = 0; i < 1000000; ++i) {
		const size_t idx = distInt(rng) % size(fns);
		if (i == 14180000) __debugbreak();
		fns[idx]();
	}
}


int
main( int argc, char** argv )
{
	TestSequences();
	TestIntervalSet();
	return 0;
}


