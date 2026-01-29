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
#include <span>
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

template<typename T> struct SequenceBase {
	T* memory = nullptr;
	size_t length = 0;

	SequenceBase() = default;
	SequenceBase(SequenceBase&& o) noexcept = default;
	SequenceBase(const SequenceBase& o) = default;
	SequenceBase<T>& operator=(const SequenceBase<T>& o) = default;
	SequenceBase<T>& operator=(SequenceBase<T>&& o) = default;
	SequenceBase(T* data_, size_t length_) : memory(data_), length(length_) {}
//	SequenceBase(const T* data_, size_t length_) : memory(const_cast<T*>(data_)), length(length_) {}

	__forceinline T* data() const { return memory; }
	__forceinline size_t size() const { return length; }
	__forceinline bool empty() const { return !length; }
	__forceinline T& front() const { assert(length); return memory[0]; }
	__forceinline T& back() const { assert(length); return memory[length-1]; }
	__forceinline T* begin() { return memory; }
	__forceinline const T* begin() const { return memory; }
	__forceinline T* end() { return memory + length; }
	__forceinline const T* end() const { return memory + length; }
	__forceinline T* rbegin() { return memory + length - 1; }
	__forceinline const T* rbegin() const { return memory + length - 1; }
	__forceinline T* rend() { return memory - 1; }
	__forceinline const T* rend() const { return memory - 1; }
	__forceinline T& operator[](size_t index) { assert(index < length); return memory[index]; }
	__forceinline const T& operator[](size_t index) const { assert(index < length); return memory[index]; }
	__forceinline const bool operator==(const SequenceBase<T>& o) {
		if (length != o.length) return false;
		for (size_t i = 0; i < length; ++i) {
			if (memory[i] != o.memory[i]) return false;
		}
		return true;
	}
	__forceinline SequenceBase<T> subseq(size_t offset, size_t count) const {
		assert(offset + count <= length);
		return SequenceBase<T>(memory + offset, count);
	}
	__forceinline SequenceBase<T> subseq(size_t offset) const {
		assert(offset <= length);
		return SequenceBase<T>(memory + offset, length - offset);
	}
};
// Unsorted sequence
template<typename T> struct Sequence : public SequenceBase<T> {
	Sequence(T* data_, size_t length_) : SequenceBase<T>(data_, length_) {}
	Sequence(const T* data_, size_t length_) : SequenceBase<T>(data_, length_) {}
	Sequence(span<T> s) : SequenceBase<T>(s.data(), s.size()) {}
	Sequence(const vector<T>& v) : SequenceBase<T>(v.data(), v.size()) {}
	template<size_t N> Sequence(const array<T, N>& a) : SequenceBase<T>(a.data(), a.size()) {}
};
// Sorted sequence
template<typename T> struct SortedSequence : public SequenceBase<T> {
	explicit SortedSequence(T* data_, size_t length_) : SequenceBase<T>(data_, length_) {}
	explicit SortedSequence(const T* data_, size_t length_) : SequenceBase<T>(data_, length_) {}
	explicit SortedSequence(Sequence<T> s) : SequenceBase<T>(s.data(), s.size()) {}
	explicit SortedSequence(span<T> s) : SequenceBase<T>(s.data(), s.size()) {}
	explicit SortedSequence(const vector<T>& v) : SequenceBase<T>(v.data(), v.size()) {}
	template<size_t N> explicit SortedSequence(const array<T, N>& a) : SequenceBase<T>(a.data(), a.size()) {}
};


template<typename T, typename LessThan> __forceinline void SortInplace(Sequence<T> pts, LessThan lessThan) {
	sort(begin(pts), end(pts), lessThan);
}
template<typename T> __forceinline void SortInplace(Sequence<T> pts) {
	sort<const T*>(begin(pts), end(pts));
}
template<typename T, typename LessThan> __forceinline vector<T> Sort(const Sequence<T> pts, LessThan lessThan) {
	vector<T> ptsS(begin(pts), end(pts));
	SortInplace<T>(ptsS, lessThan);
	return ptsS;
}
template<typename T> __forceinline vector<T> Sort(const Sequence<T> pts) {
	vector<T> ptsS(begin(pts), end(pts));
	SortInplace<T>(ptsS);
	return ptsS;
}
template<typename T, typename LessThan> __forceinline bool IsSorted(const Sequence<T> pts, LessThan lessThan) {
	const size_t cPts = pts.size();
	for (size_t i = 1; i < cPts; ++i) {
		if (lessThan(pts[i], pts[i-1])) return false;
	}
	return true;
}
template<typename T> __forceinline bool IsSorted(const Sequence<T> pts) {
	const size_t cPts = pts.size();
	for (size_t i = 1; i < cPts; ++i) {
		if (pts[i] < pts[i-1]) return false;
	}
	return true;
}

template<typename T, typename LessThan> void ZipperMerge(Sequence<T> R, const SortedSequence<T> A, const SortedSequence<T> B, LessThan lessThan) {
	assert(R.size() == A.size() + B.size());
	const auto a = begin(A);
	const auto a1 = end(A);
	const auto b = begin(B);
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
}
template<typename T> void ZipperMerge(Sequence<T> R, const SortedSequence<T> A, const SortedSequence<T> B) {
	ZipperMerge(R, A, B, std::less{});
}
template<typename T, typename LessThan> vector<T> ZipperMerge(const SortedSequence<T> A, const SortedSequence<T> B, LessThan lessThan) {
	vector<T> R;
	R.resize(A.size() + B.size());
	ZipperMerge<T>(R, A, B, lessThan);
	return R;
}
template<typename T> vector<T> ZipperMerge(const SortedSequence<T> A, const SortedSequence<T> B) {
	return ZipperMerge(A, B, std::less{});
}
template<typename T, typename LessThan> void ZipperMergeInplace(vector<T>& A, const SortedSequence<T> B, LessThan lessThan) {
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
template<typename T> void ZipperMergeInplace(vector<T>& A, const SortedSequence<T> B) {
	ZipperMergeInplace<T>(A, B, std::less{});
}


template<typename T> bool Contains(const SortedSequence<T> A, const T& b) {
	return binary_search(begin(A), end(A), b);
}

template<typename T> bool ContainsAny(const SortedSequence<T> A, const SortedSequence<T> B) {
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
template<typename T> bool ContainsAny(SortedSequence<T> A, const Sequence<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace<T>(Bs);
	return ContainsAny(A, SortedSequence<T>(Bs));
}

template<typename T> bool ContainsAll(const SortedSequence<T> A, const SortedSequence<T> B) {
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
	return true;
}
template<typename T> bool ContainsAll(const SortedSequence<T> A, const Sequence<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace<T>(Bs);
	return ContainsAll(A, SortedSequence<T>(Bs));
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
	if (!IsSorted<T>(A)) SortInplace<T>(A);
	DeduplicateSortedInplace(A);
}
template<typename T> vector<T> Deduplicate(SortedSequence<T> A) {
	vector<T> R { begin(A), end(A) };
	DeduplicateSortedInplace(R);
	return R;
}

template<typename T> void UnionInplace(vector<T>& A, const SortedSequence<T> B) {
	if (!IsSorted<T>(A)) SortInplace<T>(A);
	ZipperMergeInplace(A, B);
	DeduplicateSortedInplace(A);
}
template<typename T> vector<T> Union(SortedSequence<T> A, const SortedSequence<T> B) {
	vector<T> R = ZipperMerge(A, B);
	DeduplicateSortedInplace(R);
	return R;
}
template<typename T> vector<T> Union(SortedSequence<T> A, const Sequence<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return Union(A, SortedSequence<T>(Bs));
}

template<typename T> void IntersectInplace(vector<T>& A, const SortedSequence<T> B) {
	if (!IsSorted<T>(A)) SortInplace<T>(A);
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
template<typename T> vector<T> Intersect(SortedSequence<T> A, const SortedSequence<T> B) {
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
template<typename T> vector<T> Intersect(SortedSequence<T> A, const Sequence<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return Intersect(A, SortedSequence<T>(Bs));
}

template<typename T> void SubtractInplace(vector<T>& A, const SortedSequence<T> B) {
	if (!IsSorted<T>(A)) SortInplace<T>(A);
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
template<typename T> vector<T> Subtract(const SortedSequence<T> A, const SortedSequence<T> B) {
	vector<T> R { begin(A), end(A) };
	SubtractInplace(R, B);
	return R;
}
template<typename T> vector<T> Subtract(SortedSequence<T> A, const Sequence<T> B) {
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return Subtract(A, SortedSequence<T>(Bs));
}

// Symmetric difference
//   SymmetricDifference(A,B) = Subtract(Union(A,B), Intersection(A,B))
template<typename T> vector<T> SymmetricDifference(SortedSequence<T> A, const SortedSequence<T> B) {
	vector<T> U = Union(A, B);
	vector<T> I = Intersect(A, B);
	SubtractInplace(U, SortedSequence<T>(I));
	return U;
}
template<typename T> void SymmetricDifferenceInplace(vector<T>& A, const SortedSequence<T> B) {
	// IDEA: Union(A, B) - Intersect(A, B)
	vector<T> I = Intersect(SortedSequence<T>(A), B);
	UnionInplace(A, B);
	SubtractInplace(A, SortedSequence<T>(I));
}

template<typename T> bool Equal(const SortedSequence<T>& A, const SortedSequence<T>& B) {
	if (A.size() != B.size()) return false;
	const auto a = begin(A);
	const auto a1 = end(A);
	const auto b = begin(B);
	const auto b1 = end(B);
	return equal(a, a1, b, b1);
}


// ============================================================================
// Unsorted sequence

template<typename T> bool Contains(Sequence<T> A, const T& b) {
	for (const auto& a : A) {
		if (a == b) return true;
	}
	return false;
}

template<typename T> bool ContainsAny(Sequence<T> A, const SortedSequence<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	return ContainsAny(SortedSequence<T>(As), B);
}
template<typename T> bool ContainsAny(Sequence<T> A, const Sequence<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return ContainsAny(SortedSequence<T>(As), SortedSequence<T>(Bs));
}

template<typename T> bool ContainsAll(Sequence<T> A, const SortedSequence<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	return ContainsAll(SortedSequence<T>(As), B);
}
template<typename T> bool ContainsAll(Sequence<T> A, const Sequence<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs { begin(B), end(B) };
	SortInplace(Bs);
	return ContainsAll(SortedSequence<T>(As), SortedSequence<T>(Bs));
}

template<typename T> vector<T> DeduplicateSort(const Sequence<T> A) {
	vector<T> R { begin(A), end(A) };
	SortInplace<T>(R);
	DeduplicateSortedInplace(R);
	return R;
}
template<typename T> vector<T> Deduplicate(Sequence<T> A) {
	// ALTERNATE: Could do the same thing with DeduplicateNonOrderPreserving to make a sorted set, and then Contains(SortedSequence<T>, T) loop.
	unordered_set<T> D { begin(A), end(A) };
	vector<T> R;
	for (const auto& a : A) {
		if (D.contains(a)) R.push_back(a);
	}
	return R;
}

template<typename T> optional<size_t> IFindFirst(Sequence<T> A, const T& b) {
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		if (A[i] == b) return i;
	}
	return nullopt;
}
template<typename T> vector<size_t> IFindAll(Sequence<T> A, const T& b) {
	vector<size_t> R;
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		if (A[i] == b) R.push_back(i);
	}
	return R;
}
template<typename T> size_t Count(Sequence<T> A, const T& b) {
	size_t R = 0;
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		if (A[i] == b) ++R;
	}
	return R;
}
template<typename T> optional<size_t> IFindLast(Sequence<T> A, const T& b) {
	const size_t cA = A.size();
	for (size_t i = cA; i-- != 0; ) {
		if (A[i] == b) return i;
	}
	return nullopt;
}

template<typename T> optional<size_t> IFindFirst(SortedSequence<T> A, const T& b) {
	auto it = lower_bound(begin(A), end(A), b);
	if (it != end(A) and *it == b) {
		return it - begin(A);
	}
	return nullopt;
}
template<typename T> vector<size_t> IFindAll(SortedSequence<T> A, const T& b) {
	vector<size_t> R;
	auto it = lower_bound(begin(A), end(A), b);
	for (; it != end(A) and *it == b; ++it) {
		const size_t iStart = it - begin(A);
		R.push_back(iStart);
	}
	return R;
}
template<typename T> size_t Count(SortedSequence<T> A, const T& b) {
	size_t R = 0;
	auto it = lower_bound(begin(A), end(A), b);
	for (; it != end(A) and *it == b; ++it) {
		++R;
	}
	return R;
}
template<typename T> optional<size_t> IFindLast(SortedSequence<T> A, const T& b) {
	auto it = upper_bound(begin(A), end(A), b);
	if (it != begin(A)) {
		--it; // Move to the last occurrence.
		if (*it == b) {
			return it - begin(A);
		}
	}
	return nullopt;
}

template<typename T> optional<size_t> IReplaceFirst(Sequence<T> A, const T& find, const T& replace) {
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		if (A[i] == find) {
			A[i] = replace;
			return i;
		}
	}
	return nullopt;
}
template<typename T> bool ReplaceFirst(Sequence<T> A, const T& find, const T& replace) {
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		if (A[i] == find) {
			A[i] = replace;
			return true;
		}
	}
	return false;
}
template<typename T> vector<size_t> IReplaceAll(Sequence<T> A, const T& find, const T& replace) {
	vector<size_t> R;
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		if (A[i] == find) {
			A[i] = replace;
			R.push_back(i);
		}
	}
	return R;
}
template<typename T> bool ReplaceAll(Sequence<T> A, const T& find, const T& replace) {
	bool R = false;
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		if (A[i] == find) {
			A[i] = replace;
			R = true;
		}
	}
	return R;
}
template<typename T> optional<size_t> IReplaceLast(Sequence<T> A, const T& find, const T& replace) {
	const size_t cA = A.size();
	for (size_t i = cA; i-- != 0; ) {
		if (A[i] == find) {
			A[i] = replace;
			return i;
		}
	}
	return nullopt;
}
template<typename T> bool ReplaceLast(Sequence<T> A, const T& find, const T& replace) {
	const size_t cA = A.size();
	for (size_t i = cA; i-- != 0; ) {
		if (A[i] == find) {
			A[i] = replace;
			return true;
		}
	}
	return false;
}

// Returns the first index where the B sequence appears in A, or nullopt if not found.
template<typename T> optional<size_t> IFindFirst(Sequence<T> A, const Sequence<T>& B) {
	const size_t cA = A.size();
	const size_t cB = B.size();
	if (cB > cA) return nullopt; // B is longer than A, so it cannot be found.
	if (!cB) return nullopt; // Empty sequence is never found; arbitrary API choice.
	// IDEA: We can skip cAdvance forward after every partial match. E.g. searching for "abcabc", cAdvance=3.
	size_t cAdvance = 1;
	while (cAdvance < cB and B[0] != B[cAdvance]) {
		++cAdvance; // Advance until we find a duplicate, or reach the end of B.
	}
	const size_t iEnd = cA - cB;
	for (size_t iStart = 0; iStart <= iEnd; ) {
		if (A[iStart] != B[0]) {
			++iStart;
			continue;
		}
		bool found = true;
		for (size_t iB = 1; iB < cB; ++iB) {
			if (A[iStart + iB] != B[iB]) {
				found = false;
				break;
			}
		}
		if (found) return iStart;
		iStart += cAdvance;
	}
	return nullopt;
}
// Returns all indices where the B sequence appears in A, or empty if not found.
template<typename T> vector<size_t> IFindAll(Sequence<T> A, const Sequence<T>& B) {
	vector<size_t> R;
	const size_t cA = A.size();
	const size_t cB = B.size();
	if (cB > cA) return R; // B is longer than A, so it cannot be found.
	if (!cB) return R; // Empty sequence is never found; arbitrary API choice.
	// IDEA: We can skip cAdvance forward after every partial match. E.g. searching for "abcabc", cAdvance=3.
	size_t cAdvance = 1;
	while (cAdvance < cB and B[0] != B[cAdvance]) {
		++cAdvance; // Advance until we find a duplicate, or reach the end of B.
	}
	const size_t iEnd = cA - cB;
	for (size_t iStart = 0; iStart <= iEnd; ) {
		if (A[iStart] != B[0]) {
			++iStart;
			continue;
		}
		bool found = true;
		for (size_t iB = 1; iB < cB; ++iB) {
			if (A[iStart + iB] != B[iB]) {
				found = false;
				break;
			}
		}
		if (found) R.push_back(iStart);
		iStart += cAdvance;
	}
	return R;
}
// Returns the number of times where the B sequence appears in A.
template<typename T> size_t Count(Sequence<T> A, const Sequence<T>& B) {
	size_t R = 0;
	const size_t cA = A.size();
	const size_t cB = B.size();
	if (cB > cA) return R; // B is longer than A, so it cannot be found.
	if (!cB) return R; // Empty sequence is never found; arbitrary API choice.
	// IDEA: We can skip cAdvance forward after every partial match. E.g. searching for "abcabc", cAdvance=3.
	size_t cAdvance = 1;
	while (cAdvance < cB and B[0] != B[cAdvance]) {
		++cAdvance; // Advance until we find a duplicate, or reach the end of B.
	}
	const size_t iEnd = cA - cB;
	for (size_t iStart = 0; iStart <= iEnd; ) {
		if (A[iStart] != B[0]) {
			++iStart;
			continue;
		}
		bool found = true;
		for (size_t iB = 1; iB < cB; ++iB) {
			if (A[iStart + iB] != B[iB]) {
				found = false;
				break;
			}
		}
		if (found) ++R;
		iStart += cAdvance;
	}
	return R;
}
// Returns the last index where the B sequence appears in A, or nullopt if not found.
template<typename T> optional<size_t> IFindLast(Sequence<T> A, const Sequence<T>& B) {
	const size_t cA = A.size();
	const size_t cB = B.size();
	if (cB > cA) return nullopt; // B is longer than A, so it cannot be found.
	if (!cB) return nullopt; // Empty sequence is never found; arbitrary API choice.
	// IDEA: We can skip cAdvance backward after every partial match. E.g. searching for "abcabc", cAdvance=3.
	size_t cAdvance = 1;
	while (cAdvance < cB and B[cB - 1] != B[cB - 1 - cAdvance]) {
		++cAdvance; // Advance until we find a duplicate, or reach the end of B.
	}
	const size_t iFirst = cA - cB;
	for (size_t iA = iFirst; iA-- != 0; ) {
		if (A[iA] != B[cB - 1]) {
			continue;
		}
		bool found = true;
		for (size_t iB = 1; iB < cB; ++iB) {
			if (A[iA - iB] != B[cB - 1 - iB]) {
				found = false;
				break;
			}
		}
		if (found) return iA;
		iA = iA - (cAdvance - 1); // NOTE: Loop will decrement by additional 1.
	}
	return nullopt;
}

// TODO: IFind*(SortedSequence<T>, Sequence<T> B) - use binary search for the first element, then linear search for the rest.
// TODO: IFind*(SortedSequence<T>, SortedSequence<T> B) - use binary search for the first element, then linear search for the rest.


template<typename T> optional<T> Min(Sequence<T> A) {
	const size_t cA = A.size();
	if (!cA) return nullopt;
	T m = A[0];
	for (const size_t i = 1; i < cA; ++i) {
		if (A[i] < m) m = A[i];
	}
	return m;
}
template<typename T> optional<T> Max(Sequence<T> A) {
	const size_t cA = A.size();
	if (!cA) return nullopt;
	T m = A[0];
	for (const size_t i = 1; i < cA; ++i) {
		if (A[i] > m) m = A[i];
	}
	return m;
}
template<typename T> optional<size_t> IMinFirst(Sequence<T> A) {
	const size_t cA = A.size();
	if (!cA) return nullopt;
	size_t iMin = 0;
	T m = A[0];
	for (size_t i = 1; i < cA; ++i) {
		if (A[i] < m) {
			m = A[i];
			iMin = i;
		}
	}
	return iMin;
}
template<typename T> optional<size_t> IMaxFirst(Sequence<T> A) {
	const size_t cA = A.size();
	if (!cA) return nullopt;
	size_t iMax = 0;
	T m = A[0];
	for (size_t i = 1; i < cA; ++i) {
		if (A[i] > m) {
			m = A[i];
			iMax = i;
		}
	}
	return iMax;
}
template<typename T> vector<size_t> IMinAll(Sequence<T> A) {
	const auto m = Min(A);
	if (!m.has_value()) return {}; // Empty sequence.
	return IFindAll(A, *m);
}
template<typename T> vector<size_t> IMaxAll(Sequence<T> A) {
	const auto m = Max(A);
	if (!m.has_value()) return {}; // Empty sequence.
	return IFindAll(A, *m);
}

template<typename T> optional<T> Min(SortedSequence<T> A) {
	return A.empty() ? nullopt : A[0];
}
template<typename T> optional<T> Max(SortedSequence<T> A) {
	return A.empty() ? nullopt : A[A.size() - 1];
}
template<typename T> optional<size_t> IMinFirst(SortedSequence<T> A) {
	return A.empty() ? nullopt : 0;
}
template<typename T> optional<size_t> IMaxLast(SortedSequence<T> A) {
	return A.empty() ? nullopt : A.size() - 1;
}
// Returns the indices of all minimum elements in A (duplicates implies multiple minimums), sorted ascending.
template<typename T> vector<size_t> IMinAll(SortedSequence<T> A) {
	if (A.empty()) return {}; // Empty sequence.
	const size_t cA = A.size();
	const T& a0 = A[0];
	size_t cEqual = 1;
	while (cEqual < cA and a0 == A[cEqual]) {
		++cEqual;
	}
	vector<size_t> R(cEqual);
	iota(begin(R), end(R), 0);
	return R;
}
// Returns the indices of all maximum elements in A (duplicates implies multiple maximums), sorted descending.
template<typename T> vector<size_t> IMaxAll(SortedSequence<T> A) {
	if (A.empty()) return {}; // Empty sequence.
	const size_t cA = A.size();
	const T& a1 = A[cA - 1];
	size_t cEqual = 1;
	while (cEqual < cA and a1 == A[cA - 1 - cEqual]) {
		++cEqual;
	}
	vector<size_t> R(cEqual);
	iota(rbegin(R), rend(R), cA - cEqual);
	return R;
}

// Returns the k smallest elements in A, sorted ascending.
template<typename T> vector<T> MinK(Sequence<T> A, size_t k) {
	vector<T> R(k);
	partial_sort_copy(begin(A), end(A), begin(R), end(R));
	return R;
}
// Returns the k largest elements in A, sorted descending.
template<typename T> vector<T> MaxK(Sequence<T> A, size_t k) {
	vector<T> R(k);
	partial_sort_copy(begin(A), end(A), begin(R), end(R), greater<T>{});
	return R;
}
// Returns the indices of the k smallest elements in A, sorted ascending.
template<typename T> vector<size_t> IMinK(Sequence<T> A, size_t k) {
	// IDEA: Use a max-heap to keep the k smallest elements, so we can replace the largest of the smallest.
	const size_t cA = A.size();
	assert(k <= cA);
	vector<size_t> R;
	if (!k) return R; // Empty result.
	if (!cA) return R;
	priority_queue<pair<T, size_t>, vector<pair<T, size_t>>, greater<pair<T, size_t>>> pq;
	for (size_t i = 0; i < k; ++i) {
		pq.push({A[i], i});
	}
	for (size_t i = k; i < cA; ++i) {
		if (A[i] < pq.top().first) {
			pq.pop();
			pq.push({A[i], i});
		}
	}
	R.resize(k);
	for (size_t i = 0; i < k; ++i) {
		R[k - 1 - i] = pq.top().second;
		pq.pop();
	}
	return R;
}
// Returns the indices of the k largest elements in A, sorted descending.
template<typename T> vector<size_t> IMaxK(Sequence<T> A, size_t k) {
	// IDEA: Use a min-heap to keep the k largest elements, so we can replace the smallest of the largest.
	const size_t cA = A.size();
	assert(k <= cA);
	vector<size_t> R;
	if (!k) return R; // Empty result.
	if (!cA) return R;
	priority_queue<pair<T, size_t>> pq;
	for (size_t i = 0; i < k; ++i) {
		pq.push({A[i], i});
	}
	for (size_t i = k; i < cA; ++i) {
		if (A[i] > pq.top().first) {
			pq.pop();
			pq.push({A[i], i});
		}
	}
	R.resize(k);
	for (size_t i = 0; i < k; ++i) {
		R[k - 1 - i] = pq.top().second;
		pq.pop();
	}
	return R;
}

// Returns the order statistic for the given index k. E.g. k=0 means min, k=|A|-1 means max, k=|A|/2 means median.
// Time complexity O(|A|)
template<typename T> optional<T> KthOrderStatistic(Sequence<T> A, size_t k) {
	// IDEA: Quickselect in a copy of the sequence.
	const size_t cA = A.size();
	if (!cA) return nullopt;
	assert(k < cA);
	vector<T> As(begin(A), end(A));
	SortInplace(As);
	nth_element(begin(As), begin(As) + k, end(As));
	return As[k];
}

// TODO: RMQ


template<typename T> bool Overlaps(const Sequence<T>& A, const Sequence<T>& B) {
	const size_t a0 = static_cast<size_t>(begin(A));
	const size_t a1 = static_cast<size_t>(end(A));
	const size_t b0 = static_cast<size_t>(begin(B));
	const size_t b1 = static_cast<size_t>(end(B));
	// Assume a0 <= a1, b0 <= b1.
	// There's no overlap when a1 <= b0 (A entirely left of B), or a0 >= b1 (A entirely right of B).
	// Negation gives us the final equation.
	return !(a1 <= b0 or a0 >= b1);
}
template<typename T> void OverlapsAndStartsGreater(const Sequence<T>& A, const Sequence<T>& B) {
	const size_t a0 = static_cast<size_t>(begin(A));
	const size_t a1 = static_cast<size_t>(end(A));
	const size_t b0 = static_cast<size_t>(begin(B));
	const size_t b1 = static_cast<size_t>(end(B));
	return !(a1 <= b0 or a0 >= b1) and a0 > b0;
}
// Copies entire sequence src into dst. MUST have same length. Can overlap.
template<typename T> void Copy(Sequence<T> dst, const Sequence<T>& src) {
	assert(dst.size() == src.size());
	const size_t cA = src.size();
	if (dst.data() == src.data()) return; // No need to copy if the same.
	if (OverlapsAndStartsGreater(dst, src)) {
		// When dst > src, we must copy backwards, to preserve all values.
		for (size_t i = cA; i-- != 0; ) {
			dst[i] = src[i];
		}
	}
	else {
		for (size_t i = 0; i < cA; ++i) {
			dst[i] = src[i];
		}
	}
}
// Moves entire sequence src into dst. MUST have same length. Can overlap.
template<typename T> void Move(Sequence<T> dst, const Sequence<T>& src) {
	assert(dst.size() == src.size());
	const size_t cA = src.size();
	if (dst.data() == src.data()) return; // No need to move if the same.
	if (OverlapsAndStartsGreater(dst, src)) {
		// When dst > src, we must move backwards, to preserve all values.
		for (size_t i = cA; i-- != 0; ) {
			dst[i] = std::move(src[i]);
		}
	}
	else {
		for (size_t i = 0; i < cA; ++i) {
			dst[i] = std::move(src[i]);
		}
	}
}
// Swaps contents of two sequences. MUST have same length. MUST NOT overlap.
template<typename T> void Swap(Sequence<T> A, Sequence<T> B) {
	assert(A.size() == B.size());
	assert(!Overlaps(A, B)); // NOTE: Cannot swap overlapping sequences.
	// [0 1 2 3 4 5 6] src
	//      aaaaa      A
	//        bbbbb    B1
	// [0 1 3 ? ? 4 6]
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		swap(A[i], B[i]);
	}
}

template<typename T> void Set(Sequence<T> A, const T& b) {
	const size_t cA = A.size();
	for (size_t i = 0; i < cA; ++i) {
		A[i] = b;
	}
}

template<typename T> void ReverseInplace(Sequence<T> A) {
	// IDEA: Swap the first and last elements, then the second and second-last, etc.
	const size_t cA = A.size();
	for (size_t i = 0; i < cA / 2; ++i) {
		swap(A[i], A[cA - 1 - i]);
	}
}
template<typename T> void Reverse(Sequence<T> dst, const Sequence<T>& src) {
	assert(dst.size() == src.size());
	// IDEA: We can handle overlaps:
	// - dst > src: backwards iterate
	// - dst < src: forwards iterate
	// - dst == src: reverse in place
	// [0 1 2 3 4 5 6] src
	//      xxxxx      X
	//        yyyyy    Y1
	// [0 1 2 4 3 2 6] Reverse(Y1, X)
	//    yyyyy        Y2
	// [0 4 3 2 4 5 6] Reverse(Y2, X)
	const size_t cA = src.size();
	if (dst.data() == src.data()) {
		ReverseInplace(dst);
	}
	else if (OverlapsAndStartsGreater(dst, src)) {
		// When dst > src, we must copy backwards, to preserve all values.
		for (size_t i = cA; i-- != 0; ) {
			dst[i] = src[cA - 1 - i];
		}
	}
	else {
		for (size_t i = 0; i < cA; ++i) {
			dst[i] = src[cA - 1 - i];
		}
	}
}

template<typename T> void RotateLeftInplace(Sequence<T> A, size_t cRotate) {
#if 0
	// IDEA: Follow the unique cycle path generated by the rotation permutation.
	// [0 1 2 3 4]
	// [2 3 4 0 1] cRotate=2
	// pi(i) = (cA + i - cRotate) % cA
	// pi(0) = 3
	// pi(3) = 1
	// pi(1) = 4
	// pi(4) = 2
	// pi(2) = 0
	const size_t cA = A.size();
	if (!cA) return;
	cRotate %= cA;
	if (!cRotate) return;
	size_t i = 0;
	T tmp = std::move(A[0]);
	for (size_t iter = 0; iter < cA; ++iter) {
		const size_t iNext = (cA + i - cRotate) % cA;
		swap(tmp, A[iNext]);
		i = iNext;
	}
	assert(iNext == 0);
	swap(tmp, A[0]); // Swap the last element back to the first position.
#else
	// IDEA: Iterated reverse.
	// [0 1 2 3 4] given cRotate=2
	// [1 0 4 3 2] after ReverseInplace([0, cRotate)) and ReverseInplace([cRotate, cA))
	// [2 3 4 0 1] after ReverseInplace([0, cA))
	const size_t cA = A.size();
	if (!cA) return;
	cRotate %= cA;
	if (!cRotate) return;
	ReverseInplace(A.subseq(0, cRotate));
	ReverseInplace(A.subseq(cRotate));
	ReverseInplace(A);
#endif
}
template<typename T> void RotateRightInplace(Sequence<T> A, size_t cRotate) {
	// IDEA: Follow the unique cycle path generated by the rotation permutation.
	// [0 1 2 3 4]
	// [3 4 0 1 2] cRotate=2
	// pi(i) = (i + cRotate) % cA
	// pi(0) = 2
	// pi(2) = 4
	// pi(4) = 1
	// pi(1) = 3
	// pi(3) = 0
	const size_t cA = A.size();
	if (!cA) return;
	cRotate %= cA;
	if (!cRotate) return;
	size_t i = 0;
	T tmp = std::move(A[0]);
	for (size_t iter = 0; iter < cA; ++iter) {
		const size_t iNext = (i + cRotate) % cA;
		swap(tmp, A[iNext]);
		i = iNext;
	}
	assert(i == 0);
	swap(tmp, A[0]); // Swap the last element back to the first position.
}
template<typename T> void RotateLeft(Sequence<T> dst, const Sequence<T>& src, size_t cRotate) {
	assert(dst.size() == src.size());
	const size_t cA = src.size();
	if (!cA) return;
	cRotate %= cA;
	if (!cRotate) {
		Copy(dst, src);
		return;
	}
	Copy(dst.subseq(0, cA - cRotate), src.subseq(cRotate));
	Copy(dst.subseq(cA - cRotate), src.subseq(0, cRotate));
}
template<typename T> void RotateRight(Sequence<T> dst, const Sequence<T>& src, size_t cRotate) {
	assert(dst.size() == src.size());
	const size_t cA = src.size();
	if (!cA) return;
	cRotate %= cA;
	if (!cRotate) {
		Copy(dst, src);
		return;
	}
	Copy(dst.subseq(0, cRotate), src.subseq(cA - cRotate));
	Copy(dst.subseq(cRotate), src.subseq(0, cA - cRotate));
}

template<typename T, typename FnKeepT> Sequence<T> FilterInplaceNonOrderPreserving(Sequence<T> A, FnKeepT&& fnKeep) {
	// IDEA: two iterators, one forward looking for !keep, one reverse looking for keep. Move element when both hit.
	if (A.empty()) return A;
	auto a = begin(A);
	auto a1 = end(A);
	#if 1
	for (; a != a1; ++a) {
		const bool keep = fnKeep(*a);
		if (!keep) {
			do {
				--a1;
				const bool keepEnd = fnKeep(*a1);
				if (keepEnd) break;
			} while (a != a1);
			if (a == a1) break;
			*a = std::move(*a1);
		}
	}
	#else
	for (;;) {
		while (a != a1 and fnKeep(*a)) {
			++a;
		}
		if (a == a1) break;
		--a1;
		while (a != a1 and !fnKeep(*a1)) {
			--a1;
		}
		if (a == a1) break;
		*a = std::move(*a1);
		++a;
	}
	#endif
	const size_t cNew = a1 - begin(A);
	return { A.data(), cNew };
}
template<typename T, typename FnKeepT> Sequence<T> FilterInplace(Sequence<T> A, FnKeepT&& fnKeep) {
	// IDEA: two iterators, read and write. Advance read every time, advance write and move when keeps says to.
	if (A.empty()) return A;
	auto a = begin(A);
	auto b = begin(A);
	auto a1 = end(A);
	for (; a != a1; ++a) {
		const bool keep = fnKeep(*a);
		if (keep) {
			if (a != b) {
				*b = std::move(*a);
			}
			++b;
		}
	}
	const size_t cNew = b - begin(A);
	return { A.data(), cNew };
}
template<typename T, typename FnKeepT> vector<T> Filter(Sequence<T> A, FnKeepT&& fnKeep) {
	vector<T> R;
	for (const auto& a : A) {
		const bool keep = fnKeep(a);
		if (keep) R.push_back(a);
	}
	return R;
}

template<typename T> vector<T> Union(Sequence<T> A, const SortedSequence<T> B) {
	return Union(B, A);
}
template<typename T> vector<T> Union(Sequence<T> A, const Sequence<T> B) {
	vector<T> As { begin(A), end(A) };
	SortInplace(As);
	return Union(As, B);
}

template<typename T> vector<T> Intersect(Sequence<T> A, const SortedSequence<T> B) {
	return Intersect(B, A);
}
template<typename T> vector<T> Intersect(Sequence<T> A, const Sequence<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	return Intersect(SortedSequence<T>(As), B);
}

template<typename T> vector<T> Subtract(Sequence<T> A, const SortedSequence<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	return Subtract(SortedSequence<T>(As), B);
}
template<typename T> vector<T> Subtract(Sequence<T> A, const Sequence<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs{ begin(B), end(B) };
	SortInplace(Bs);
	return Subtract(SortedSequence<T>(As), SortedSequence<T>(Bs));
}

template<typename T> vector<T> SymmetricDifference(Sequence<T> A, const SortedSequence<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	return SymmetricDifference(SortedSequence<T>(As), B);
}
template<typename T> vector<T> SymmetricDifference(Sequence<T> A, const Sequence<T> B) {
	vector<T> As{ begin(A), end(A) };
	SortInplace(As);
	vector<T> Bs{ begin(B), end(B) };
	SortInplace(Bs);
	return SymmetricDifference(SortedSequence<T>(As), SortedSequence<T>(Bs));
}

template<typename T> bool Equal(const Sequence<T>& A, const Sequence<T>& B) {
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
	explicit NaiveSet(const Sequence<int> points_) {
		for (const auto& p : points_) {
			points.insert(p);
		}
	}
};
bool Contains(const NaiveSet& A, int pt) {
	return A.points.contains(pt);
}
bool ContainsAny(const NaiveSet& A, const Sequence<int> pts) {
	for (const auto& p : pts) {
		if (Contains(A, p)) return true;
	}
	return false;
}
bool ContainsAll(const NaiveSet& A, const Sequence<int> pts) {
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
bool Equal(const NaiveSet& A, const Sequence<int> B) {
	NaiveSet Bs{ B };
	return Equal(A, Bs);
}
bool Equal(const Sequence<int> A, const NaiveSet& B) {
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
				const auto Sc = Contains<int>(SortedSequence<int>(S), p);
				assert(Rc == Sc);
			}
		},
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			const auto Rc = ContainsAny(R, pts);
			const auto Sc = ContainsAny<int>(SortedSequence<int>(S), Sequence<int>(pts));
			assert(Rc == Sc);
		},
		[&]() {
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			const auto Rc = ContainsAll(R, pts);
			const auto Sc = ContainsAll<int>(SortedSequence<int>(S), Sequence<int>(pts));
			assert(Rc == Sc);
		},
		[&]() {
			// Intersect
			assert(Equal(S, R));
			vector<int> SI = Intersect(SortedSequence<int>(S), SortedSequence<int>(S));
			assert(Equal(SortedSequence<int>(SI), SortedSequence<int>(S)));
			NaiveSet RI = Intersect(R, R);
			assert(Equal(RI, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(Sequence<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Intersect(SortedSequence<int>(S), SortedSequence<int>(Sr));
			auto Rn = Intersect(R, Rr);
			assert(Equal(Sn, Rn));
			IntersectInplace(S, SortedSequence<int>(Sr));
			assert(Equal(SortedSequence<int>(S), SortedSequence<int>(Sn)));
			R = Rn;
		},
		[&]() {
			// Subtract
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(Sequence<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Subtract<int>(SortedSequence<int>(S), SortedSequence<int>(Sr));
			auto Rn = Subtract(R, Rr);
			assert(Equal(Sn, Rn));
			SubtractInplace(S, SortedSequence<int>(Sr));
			assert(Equal(SortedSequence<int>(S), SortedSequence<int>(Sn)));
			R = Rn;
		},
		[&]() {
			// Union
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(Sequence<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = Union<int>(SortedSequence<int>(S), SortedSequence<int>(Sr));
			auto Rn = Union(R, Rr);
			assert(Equal(Sn, Rn));
			UnionInplace<int>(S, SortedSequence<int>(Sr));
			assert(Equal(SortedSequence<int>(S), SortedSequence<int>(Sn)));
			R = Rn;
		},
		[&]() {
			// SymmetricDifference
			assert(Equal(S, R));
			vector<int> pts = randomInts();
			vector<int> Sr = DeduplicateSort(Sequence<int>(pts));
			NaiveSet Rr { pts };
			assert(Equal(Sr, Rr));
			auto Sn = SymmetricDifference(SortedSequence<int>(S), SortedSequence<int>(Sr));
			auto Rn = SymmetricDifference(R, Rr);
			assert(Equal(Sn, Rn));
			SymmetricDifferenceInplace<int>(S, SortedSequence<int>(Sr));
			assert(Equal(SortedSequence<int>(S), SortedSequence<int>(Sn)));
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

template<typename T> struct IntervalII {
	T p0, p1;

//	IntervalII(const IntervalII<T>& o) = default;
//	IntervalII(IntervalII<T>&& o) = default;
};
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
template<typename T> __forceinline bool ContainsAny(const IntervalII<T>& A, const Sequence<T> pts) {
	for (const auto& p : pts) {
		if (Contains(A, p)) return true;
	}
	return false;
}
// Interval contains all of the given points.
template<typename T> __forceinline bool ContainsAll(const IntervalII<T>& A, const Sequence<T> pts) {
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

template<typename T> vector<IntervalII<T>> Merge(const Sequence<IntervalII<T>> intervals) {
	auto MergeSorted = [](const Sequence<IntervalII<T>> intervals) {
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

template<typename T> vector<IntervalII<T>> SortedIntervalsFromPoints(const Sequence<T> points) {
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
	explicit IntervalSet(const Sequence<IntervalII<T>> intervals_) {
		intervals = Merge(intervals_);
	}
	explicit IntervalSet(const IntervalII<T>& interval_) {
		const Sequence<IntervalII<T>> intervals_{ (IntervalII<T>*)&interval_, 1 };
		intervals = Merge(intervals_);
	}
	explicit IntervalSet(const Sequence<T> points) {
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
template<typename T> void UnionInplace(IntervalSet<T>& A, const SortedSequence<IntervalII<T>> toInsert) {
	if (toInsert.empty()) return;
	ZipperMergeInplace<IntervalII<T>>(A.intervals, toInsert, LessThanStartIntervalII<T>{});
	MergeSortedInplace(A.intervals);
};
template<typename T> void UnionInplace(IntervalSet<T>& A, const Sequence<IntervalII<T>> toInsert) {
	if (IsSorted(toInsert, LessThanStartIntervalII<T>{})) {
		UnionInplace(A, SortedSequence<IntervalII<T>>(toInsert));
	}
	else {
		auto toInsertS = Sort(toInsert, LessThanStartIntervalII<T>{});
		UnionInplace(A, SortedSequence<IntervalII<T>>(toInsertS));
	}
}
template<typename T> void UnionInplace(IntervalSet<T>& A, const IntervalSet<T>& B) {
	UnionInplace<T>(A, B.intervals);
}
template<typename T> IntervalSet<T> Union(const IntervalSet<T>& A, const IntervalSet<T>& B) {
	// IDEA: Zipper merge the two sorted interval sets, then merge adjacents/overlaps.
	// This is O(|A| + |B|)
	IntervalSet<T> result;
	result.intervals = ZipperMerge<IntervalII<T>>(SortedSequence<IntervalII<T>>(A.intervals), SortedSequence<IntervalII<T>>(B.intervals), LessThanStartIntervalII<T>{});
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
template<typename T> bool ContainsAnySorted(const IntervalSet<T>& A, const Sequence<T> pointsSorted) {
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
template<typename T> bool ContainsAny(const IntervalSet<T>& A, const Sequence<T> pts) {
	// Alternative: for each given point, binary search the intervals. This would be O(|P| log |I|).
	if (IsSorted<T>(pts)) {
		return ContainsAnySorted<T>(A, pts);
	}
	else {
		vector<T> sorted = Sort<T>(pts);
		return ContainsAnySorted<T>(A, sorted);
	}
}
template<typename T> bool ContainsAllSorted(const IntervalSet<T>& A, const Sequence<T> pointsSorted) {
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
template<typename T> bool ContainsAll(const IntervalSet<T>& A, const Sequence<T> pts) {
	// Alternative: for each given point, binary search the intervals. This would be O(|P| log |I|).
	if (IsSorted<T>(pts)) {
		return ContainsAllSorted<T>(A, pts);
	}
	else {
		vector<T> sorted = Sort<T>(pts);
		return ContainsAllSorted<T>(A, sorted);
	}
}

template<typename T> optional<IntervalII<T>> Intersect(const Sequence<IntervalII<T>> intervals) {
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
	for (size_t i = 0; i < 1000; ++i) {
		const size_t idx = distInt(rng) % size(fns);
		if (i == 14180) __debugbreak();
		fns[idx]();
	}
}



struct Bitmap
{
	vector<uint64_t> m_v;
	size_t m_cBits = 0;

	__forceinline size_t size() const noexcept { return m_cBits; }
	__forceinline bool empty() const noexcept { return m_cBits == 0; }
	__forceinline void clear() noexcept
	{
		m_cBits = 0;
		m_v.clear();
	}
	__forceinline void resize(size_t cBits)
	{
		m_cBits = cBits;
		m_v.resize((cBits + 63) / 64);

		// Reset the trailing bits in the last word.
		if (cBits)
		{
			const size_t j = cBits - 1;
			const size_t j64 = j / 64;
			const size_t jbit = j % 64;
			const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
			m_v[j64] &= jmask;
		}
	}
	__forceinline bool get(size_t i) const noexcept
	{
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		return (m_v[i64] & (1ULL << ibit)) != 0;
	}
	__forceinline void set(size_t i, bool f) noexcept
	{
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		if (f)
			m_v[i64] |= (1ULL << ibit);
		else
			m_v[i64] &= ~(1ULL << ibit);
	}
	__forceinline void set(size_t i) noexcept
	{
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		m_v[i64] |= (1ULL << ibit);
	}
	__forceinline void reset(size_t i) noexcept
	{
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		m_v[i64] &= ~(1ULL << ibit);
	}
	__forceinline bool getThenSet(size_t i) noexcept
	{
		assert(i < m_cBits);
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		const bool f = (m_v[i64] & (1ULL << ibit)) != 0;
		m_v[i64] |= (1ULL << ibit);
		return f;
	}
	// Sets the range [i, j] to 1.
	__forceinline void setRange(size_t i, size_t j) noexcept
	{
		assert(i <= j);
		assert(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		if (i64 == j64)
		{
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			m_v[i64] |= mask;
		}
		else
		{
			// Set the bits in the first word.
			m_v[i64] |= ~imask;
			// Set the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				m_v[k] = ~0ULL;
			// Set the bits in the last word.
			m_v[j64] |= jmask;
		}
	}
	__forceinline void setAll() noexcept
	{
		if (m_cBits)
			setRange(0, m_cBits - 1);
	}
	// Resets the range [i, j] to 0.
	__forceinline void resetRange(size_t i, size_t j) noexcept
	{
		assert(i <= j);
		assert(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		if (i64 == j64)
		{
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			m_v[i64] &= ~mask;
		}
		else
		{
			// Reset the bits in the first word.
			m_v[i64] &= imask;
			// Reset the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				m_v[k] = 0ULL;
			// Reset the bits in the last word.
			m_v[j64] &= ~jmask;
		}
	}
	__forceinline void resetAll() noexcept
	{
		if (m_cBits)
			resetRange(0, m_cBits - 1);
	}
	// Returns the number of bits set to 1 in the range [i, j].
	__forceinline size_t popcount(size_t i, size_t j) const noexcept
	{
		assert(i <= j);
		assert(j < m_cBits);
		const size_t i64 = i / 64;
		const size_t j64 = j / 64;
		const size_t ibit = i % 64;
		const size_t jbit = j % 64;
		const uint64_t imask = ((1ULL << ibit) - 1);
		const uint64_t jmask = jbit == 63 ? ~0ULL : (1ULL << (jbit + 1)) - 1;
		size_t count = 0;
		if (i64 == j64)
		{
			// All bits are in the same 64-bit word.
			const uint64_t mask = imask ^ jmask;
			count += std::popcount(m_v[i64] & mask);
		}
		else
		{
			// Count the bits in the first word.
			count += std::popcount(m_v[i64] & ~imask);
			// Count the bits in the middle words.
			for (size_t k = i64 + 1; k < j64; ++k)
				count += std::popcount(m_v[k]);
			// Count the bits in the last word.
			count += std::popcount(m_v[j64] & jmask);
		}
		return count;
	}
	// Appends the contents of another bitmap to this one.
	__forceinline void append(const Bitmap& o)
	{
		if (o.empty())
			return;
		if (empty())
		{
			*this = o;
			return;
		}
		const size_t cold = m_cBits / 64;
		const size_t cshift = (m_cBits % 64);
		if (!cshift)
		{
			m_v.insert(end(m_v), begin(o.m_v), end(o.m_v));
			m_cBits += o.m_cBits;
			return;
		}
		const size_t calign = 64 - (m_cBits % 64);
		const size_t mask = calign == 64 ? ~0ULL : ((1ULL << calign) - 1);
		m_v.resize((m_cBits + o.m_cBits + 63) / 64);
		m_cBits += o.m_cBits;
		// Handle all complete words from source
		size_t i = 0;
		for (; i < o.m_v.size() - 1; ++i)
		{
			m_v[cold + i] |= (o.m_v[i] & mask) << cshift;
			m_v[cold + i + 1] |= (o.m_v[i] >> calign);
		}
		// Handle the last word specially to respect o.m_cBits
		const size_t crem = o.m_cBits % 64;
		const uint64_t lastMask = crem == 0 ? ~0ULL : ((1ULL << crem) - 1);
		const uint64_t lastWord = o.m_v[i] & lastMask;
		m_v[cold + i] |= (lastWord & mask) << cshift;
		if ((cold + i + 1) < m_v.size())
			m_v[cold + i + 1] |= (lastWord >> calign);
	}
	__forceinline void emplace_back(bool f)
	{
		const size_t i = m_cBits++;
		const size_t i64 = i / 64;
		const size_t ibit = i % 64;
		if (i64 == m_v.size())
		{
			m_v.push_back(0);
		}
		m_v[i64] |= (size_t)f << ibit;
	}

	Bitmap() = default;
	Bitmap(size_t cBits) : m_cBits(cBits)
	{
		m_v.resize((cBits + 63) / 64);
	}
	~Bitmap() noexcept = default;
	Bitmap(const Bitmap& o) : m_v(o.m_v), m_cBits(o.m_cBits) {}
	Bitmap& operator=(const Bitmap& o)
	{
		m_v = o.m_v;
		m_cBits = o.m_cBits;
		return *this;
	}
	Bitmap(Bitmap&& o) noexcept
	{
		// clang-format off
		m_v = std::move(o.m_v); o.m_v.clear();
		m_cBits = std::move(o.m_cBits); o.m_cBits = 0;
		// clang-format on
	}
	Bitmap& operator=(Bitmap&& o) noexcept
	{
		// clang-format off
		m_v = std::move(o.m_v); o.m_v.clear();
		m_cBits = std::move(o.m_cBits); o.m_cBits = 0;
		// clang-format on
		return *this;
	}
};



int
main( int argc, char** argv )
{
	TestSequences();
	TestIntervalSet();
	return 0;
}


