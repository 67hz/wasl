#include <wasl/vproxy_ptr.h>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

using wasl::vproxy_ptr;

class non_trivial {
public:
  non_trivial(int num) : _num{num} {}

  int num() const { return _num; }

private:
  int _num;
};

struct nontrivial_container {
  non_trivial ntc;
};

struct virtual_proxied_nontrivial_container {
  vproxy_ptr<non_trivial> &proxy() { return ntc; }
  vproxy_ptr<non_trivial> ntc;
};

struct trivial {
	trivial() = default;
	trivial(int x) : i{x} {}
	int i {1};
};

TEST(vproxy_ptrNontrivialClass, IsDefaultConstructibleWithvproxy_ptr) {
  ASSERT_FALSE(std::is_default_constructible<non_trivial>::value);
  ASSERT_TRUE(
      std::is_default_constructible<vproxy_ptr<non_trivial>>::value);
};

TEST(vproxy_ptrNontrivialContainer, IsDefaultConstructible) {
  ASSERT_FALSE(std::is_default_constructible<nontrivial_container>::value);
  ASSERT_TRUE(
      std::is_default_constructible<vproxy_ptr<nontrivial_container>>::value);
}

TEST(vproxy_ptrNontrivialContainer, CanDeferNonDefaultConstruction) {
  vproxy_ptr<non_trivial> vp;
  vp.load(34);
  ASSERT_EQ(vp->num(), 34);
}

TEST(vproxy_ptrNontrivialContainer, CanStoreParamsAndDeferNonDefaultConstruction) {
  vproxy_ptr<non_trivial> vp(34);
  vp.load();
  ASSERT_EQ(vp->num(), 34);
}

TEST(vproxy_ptrState, BoolComparisonReturnsFalseBeforeLoaded) {
  non_trivial ntc1(1);
  vproxy_ptr<non_trivial> vp1;
  ASSERT_TRUE(!vp1);
}

TEST(vproxy_ptrState, BoolComparisonReturnsTrueAfterLoaded) {
  non_trivial ntc1(1);
  vproxy_ptr<non_trivial> vp1;
  vp1.load(ntc1);
  ASSERT_TRUE(!!vp1);
}

TEST(vproxy_ptrOps, ArrowOperator) {
  non_trivial ntc1(1);
  vproxy_ptr<non_trivial> vp1;
  vp1.load(ntc1);
  ASSERT_EQ(vp1->num(), 1);
}

TEST(vproxy_ptrOps, StarOperator) {
  non_trivial ntc1(1);
  vproxy_ptr<non_trivial> vp1;
  vp1.load(ntc1);
  ASSERT_EQ((*vp1).num(), 1);
}

TEST(vproxy_ptrConstructor, IsNotCopyConstructible) {
	vproxy_ptr<trivial> t1;
  vproxy_ptr<non_trivial> nt1;
	ASSERT_FALSE(std::is_copy_constructible<vproxy_ptr<trivial>>::value);
	ASSERT_FALSE(std::is_copy_constructible<vproxy_ptr<non_trivial>>::value);
}

TEST(vproxy_ptrConstructor, IsMoveConstructible) {
  non_trivial ntc1(1);
  vproxy_ptr<non_trivial> vp1;
  vp1.load(ntc1);

  vproxy_ptr<non_trivial> vp2(std::move(vp1));
  ASSERT_EQ(vp2->num(), 1);
  ASSERT_TRUE(!!vp2);
  ASSERT_TRUE(!vp1);
}

TEST(vproxy_ptrConstructor, CanMoveConstructWithAClosure) {
  vproxy_ptr<non_trivial> vp1(14);
  vproxy_ptr<non_trivial> vp2(std::move(vp1));
  vp2.load();
  ASSERT_TRUE(!!vp2);
  ASSERT_TRUE(!vp1);
  ASSERT_EQ(vp2->num(), 14);
}

TEST(vproxy_ptrAssignment, IsMoveAssignableWithExistingDestNode) {
  non_trivial ntc1(1);
  vproxy_ptr<non_trivial> vp1;
  vp1.load(ntc1);

  vproxy_ptr<non_trivial> vp2;
  non_trivial ntc2(2);
  vp2.load(ntc2);
  vp2 = (std::move(vp1));
  ASSERT_EQ(vp2->num(), 1);
  ASSERT_TRUE(!!vp2);
  ASSERT_TRUE(!vp1);
}

TEST(vproxy_ptrAssignment, IsMoveAssignableWithEmptyDestNode) {
  non_trivial ntc1(1);
  vproxy_ptr<non_trivial> vp1;
  vp1.load(ntc1);

  vproxy_ptr<non_trivial> vp2;
  vp2 = (std::move(vp1));
  ASSERT_EQ(vp2->num(), 1);
  ASSERT_TRUE(!!vp2);
  ASSERT_TRUE(!vp1);
}

TEST(vproxy_ptrAssignment, CanMoveAssignWithAClosure) {
  vproxy_ptr<non_trivial> vp1(8);
  vproxy_ptr<non_trivial> vp2;
  vp2 = (std::move(vp1));
  vp2.load();
  ASSERT_TRUE(!!vp2);
  ASSERT_TRUE(!vp1);
  ASSERT_EQ(vp2->num(), 8);
}



TEST(vproxy_ptrLoadingDefaultConstructible, CanLoadWithNoArgs) {
	ASSERT_TRUE(std::is_default_constructible<trivial>::value);
	vproxy_ptr<trivial> t1;
	ASSERT_FALSE(t1);
	t1.load();
	ASSERT_EQ(t1->i, 1);
}

TEST(vproxy_ptrLoadingDefaultConstructible, CanLoadWithNoArgsAndClosure) {
	ASSERT_TRUE(std::is_default_constructible<trivial>::value);
	vproxy_ptr<trivial> t1(2);
	t1.load();
	ASSERT_EQ(t1->i, 2);
}

TEST(vproxy_ptrLoadingDefaultConstructible,
		LoadWithArgsUpdatesProxy) {
	vproxy_ptr<trivial> t1;
	t1.load(2);
	ASSERT_EQ(t1->i, 2);
}

TEST(vproxy_ptrLoadingDefaultConstructible,
		LoadWithArgsAfterPreviouslyLoadedDoesNotUpdateProxy) {
	vproxy_ptr<trivial> t1;
	t1.load(2);
	t1.load(3);
	ASSERT_EQ(t1->i, 2);
}

TEST(vproxy_ptrLoadingDefaultConstructible,
		LoadWithArgsWithClosureUpdatesProxy) {
	vproxy_ptr<trivial> t1(2);
	t1.load(3);
	ASSERT_EQ(t1->i, 3);
}

TEST(vproxy_ptrLoadingDefaultConstructible,
		ReLoadArgsWithClosureDoesNotUpdateProxy) {
	vproxy_ptr<trivial> t1(2);
	t1.load();
	t1.load(3);
	ASSERT_NE(t1->i, 3);
}

TEST(vproxy_ptrLoadingNonDefaultConstructible,
		CannotLoadWithNoArgs) {
  ASSERT_FALSE(std::is_default_constructible<non_trivial>::value);
	vproxy_ptr<non_trivial> ntc;
	ASSERT_THROW( {
		ntc.load();
	}, std::runtime_error);
}

TEST(vproxy_ptrLoadingNonDefaultConstructible,
		LoadWithArgsAfterPreviouslyLoadedDoesNotUpdateProxy) {
  ASSERT_FALSE(std::is_default_constructible<non_trivial>::value);
	vproxy_ptr<non_trivial> ntc;
	ntc.load(1);
	ntc.load(2);
	ASSERT_EQ(ntc->num(), 1);
}

TEST(vproxy_ptrLoadingNonDefaultConstructible,
		LoadWithArgsWithClosureUpdatesProxy) {
	vproxy_ptr<non_trivial> ntc(1);
	ntc.load(3);
	ASSERT_EQ(ntc->num(), 3);
}

TEST(vproxy_ptrLoadingNonDefaultConstructible,
		ReLoadArgsWithClosureDoesNotUpdateProxy) {
	vproxy_ptr<non_trivial> ntc(1);
	ntc.load();
	ntc.load(2);
	ASSERT_EQ(ntc->num(), 1);
}

