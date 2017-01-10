#ifndef SUBHALO_HEADER_INCLUDED
#define SUBHALO_HEADER_INCLUDED

#include <iostream>
#include <new>
#include <vector>

#include "datatypes.h"
#include "snapshot_number.h"
#include "halo.h"
#include "hdf_wrapper.h"

enum class SubReaderDepth_t
{
  SubTable,//read only the properties of subhalos
  SubParticles,//read sub particle list, plus properties of sub
  SrcParticles//read src particle list instead of sub particles, plus properties of sub 
};

class Subhalo_t;
typedef vector <Subhalo_t> SubhaloList_t;

class Subhalo_t
{
public:
  typedef vector <HBTInt> ParticleList_t;
  typedef vector <HBTInt> SubIdList_t;
  HBTInt TrackId;
  HBTInt Nbound;
  float Mbound;
#ifndef DM_ONLY
  HBTInt NboundType[TypeMax];
  float MboundType[TypeMax];
#endif
  HBTInt HostHaloId;
  HBTInt Rank;
  float LastMaxMass;
  int SnapshotIndexOfLastMaxMass; //the snapshot when it has the maximum subhalo mass, only considering past snapshots.
  int SnapshotIndexOfLastIsolation; //the last snapshot when it was a central, only considering past snapshots.
  
  int SnapshotIndexOfBirth;//when the subhalo first becomes resolved
  int SnapshotIndexOfDeath;//when the subhalo first becomes un-resolved; only set if currentsnapshot>=SnapshotIndexOfDeath.
  
  //profile properties
  float RmaxComoving;
  float VmaxPhysical;
  float LastMaxVmaxPhysical;
  int SnapshotIndexOfLastMaxVmax; //the snapshot when it has the maximum Vmax, only considering past snapshots.
  
  float R2SigmaComoving; //95.5% containment radius, close to tidal radius?
  float RHalfComoving;
  
  //SO properties using subhalo particles alone
  float R200CritComoving;
  float R200MeanComoving;
  float RVirComoving;
  float M200Crit;
  float M200Mean;
  float MVir;
  
  //kinetic properties
  float SpecificSelfPotentialEnergy;
  float SpecificSelfKineticEnergy;//<0.5*v^2>
  float SpecificAngularMomentum[3];//<Rphysical x Vphysical>
  float SpinPeebles[3];
  float SpinBullock[3];
  
  //shapes
#ifdef HAS_GSL
  float InertialEigenVector[3][3];//three float[3] vectors.
  float InertialEigenVectorWeighted[3][3];
#endif
  float InertialTensor[6]; //{Ixx, Ixy, Ixz, Iyy, Iyz, Izz}
  float InertialTensorWeighted[6];
  
  HBTxyz ComovingAveragePosition;
  HBTxyz PhysicalAverageVelocity;//default vel of sub
  HBTxyz ComovingMostBoundPosition;//default pos of sub
  HBTxyz PhysicalMostBoundVelocity;
  
  HBTxyz ComovingCorePosition, PhysicalCoreVelocity;
  float ComovingCoreSigmaR, PhysicalCoreSigmaV;
  
  ParticleList_t Particles;
#ifdef SAVE_BINDING_ENERGY
  vector <float> Energies;
#endif
  SubIdList_t NestedSubhalos;//list of sub-in-subs.
  
  Subhalo_t(): Nbound(0), Rank(0), Mbound(0)
#ifndef DM_ONLY
  ,NboundType{0}, MboundType{0.}
#endif
  {
	TrackId=SpecialConst::NullTrackId;
	SnapshotIndexOfLastIsolation=SpecialConst::NullSnapshotId;
	SnapshotIndexOfLastMaxMass=SpecialConst::NullSnapshotId;
	LastMaxMass=0;
	LastMaxVmaxPhysical=0.;
	SnapshotIndexOfLastMaxVmax=SpecialConst::NullSnapshotId;
	SnapshotIndexOfBirth=SpecialConst::NullSnapshotId;
	SnapshotIndexOfDeath=SpecialConst::NullSnapshotId;
  }
  /*void MoveTo(Subhalo_t & dest)
  {//override dest with this, leaving this unspecified.
	dest.TrackId=TrackId;
	dest.Nbound=Nbound;
	dest.HostHaloId=HostHaloId;
	dest.Rank=Rank;
	dest.LastMaxMass=LastMaxMass;
	dest.SnapshotIndexOfLastMaxMass=SnapshotIndexOfLastMaxMass;
	dest.SnapshotIndexOfLastIsolation=SnapshotIndexOfLastIsolation;
	copyHBTxyz(dest.ComovingPosition, ComovingPosition);
	copyHBTxyz(dest.PhysicalVelocity, PhysicalVelocity);
	dest.Particles.swap(Particles);
  }*/
  void Unbind(const ParticleSnapshot_t &part_snap);
  void TruncateSource();
  void RecursiveUnbind(SubhaloList_t &Subhalos, const ParticleSnapshot_t &snap);
  HBTReal KineticDistance(const Halo_t & halo, const ParticleSnapshot_t & partsnap);
  float GetMass() const
  {
	return Mbound; //accumulate(begin(MboundType), end(MboundType), (HBTReal)0.);
  }
  void UpdateTrack(const ParticleSnapshot_t &part_snap);
  bool IsCentral()
  {
	return 0==Rank;
  }
  void CalculateProfileProperties(const ParticleSnapshot_t &part_snap);
  void CalculateShape(const ParticleSnapshot_t &part_snap);
  void CountParticleTypes(const ParticleSnapshot_t &part_snap);
  HBTInt ParticleIdToIndex(const ParticleSnapshot_t &part_snap);
//   void SetHostHalo(const vector <HBTInt> &ParticleToHost);
  void LevelUpDetachedMembers(vector <Subhalo_t> &Subhalos);
};

class MemberShipTable_t
/* list the subhaloes inside each host, rather than ordering the subhaloes 
 * 
 * the principle is to not move the objects, but construct a table of them, since moving objects will change their id (or index at least), introducing the trouble to re-index them and update the indexes in any existence references.
 */
{
public:
  typedef VectorView_t <HBTInt> MemberList_t;  //list of members in a group
private:
  void BindMemberLists();
  void FillMemberLists(const SubhaloList_t & Subhalos, bool include_orphans);
  void CountMembers(const SubhaloList_t & Subhalos, bool include_orphans);
  void SortSatellites(const SubhaloList_t & Subhalos);
  void CountEmptyGroups();
  /*avoid operating on the Mem_* below; use the public VectorViews whenever possible; only operate the Mem_* variables when adjusting memory*/
  vector <MemberList_t> Mem_SubGroups; //list of subhaloes inside each host halo, with the storage of each subgroup mapped to a location in AllMembers 
public:
  vector <HBTInt> AllMembers; //the complete list of all the subhaloes in SubGroups. do not resize this vector manually. resize only with ResizeAllMembers() function.
  VectorView_t <MemberList_t> SubGroups; //list of subhaloes inside each host halo. contain one more group than halo catalogue, to hold field subhaloes. It is properly offseted so that SubGroup[hostid=-1] gives field subhaloes, and hostid>=0 for the normal groups.
  HBTInt NBirth; //newly born halos, excluding fake halos
  HBTInt NFake; //Fake (unbound) halos with no progenitors
  vector < vector<HBTInt> > SubGroupsOfHeads; //list of top-level subhaloes in each halo
  
  MemberShipTable_t(): Mem_SubGroups(), AllMembers(), SubGroups(), SubGroupsOfHeads(), NBirth(0), NFake(0)
  {
  }
  HBTInt GetNumberOfFieldSubs()
  {
	return SubGroups[-1].size();
  }
  void Init(const HBTInt nhalos, const HBTInt nsubhalos, const float alloc_factor=1.2);
  void ResizeAllMembers(size_t n);
  void Build(const HBTInt nhalos, const SubhaloList_t & Subhalos, bool include_orphans);
  void SortMemberLists(const SubhaloList_t & Subhalos);
  void AssignRanks(SubhaloList_t &Subhalos);
  void SubIdToTrackId(const SubhaloList_t &Subhalos);
  void TrackIdToSubId(SubhaloList_t &Subhalos);
};
class SubhaloSnapshot_t: public Snapshot_t
{ 
private:
  void RegisterNewTracks();
  bool ParallelizeHaloes;
  hid_t H5T_SubhaloInMem, H5T_SubhaloInDisk;
  
  void DecideCentrals(const HaloSnapshot_t &halo_snap);
  void FeedCentrals(HaloSnapshot_t &halo_snap);
  void BuildHDFDataType();
  void PurgeMostBoundParticles();
  void LevelUpDetachedSubhalos();
  void ExtendCentralNest();
  void NestSubhalos();
  void MaskSubhalos();
  void ReadFile(int iFile, const SubReaderDepth_t depth);
  void LoadSubDir(int snapshot_index, const SubReaderDepth_t depth);
  void LoadSingle(int snapshot_index, const SubReaderDepth_t depth);
  HBTInt GetNumberOfSubhalos(int iFile);
  vector <HBTInt> FileOffset;
public:
  const ParticleSnapshot_t * SnapshotPointer;
  SubhaloList_t Subhalos;
  MemberShipTable_t MemberTable;
  SubhaloSnapshot_t(): Snapshot_t(), Subhalos(), MemberTable(), SnapshotPointer(nullptr),  ParallelizeHaloes(true)
  {
	BuildHDFDataType();
  }
  SubhaloSnapshot_t(int snapshot_index, const SubReaderDepth_t depth=SubReaderDepth_t::SubParticles): SubhaloSnapshot_t()
  {
	Load(snapshot_index, depth);
  }
  string GetSubDir();
  void GetSubFileName(string &filename, int iFile, const string &filetype="Sub");  
  void GetSubFileName(string &filename, const string &filetype="Sub");
  void Load(int snapshot_index, const SubReaderDepth_t depth=SubReaderDepth_t::SubParticles);
  void Save();
  ~SubhaloSnapshot_t();
  void Clear()
  {
	//TODO
	cout<<"Clean() not implemented yet\n";
  }
  void ParticleIdToIndex(const ParticleSnapshot_t & snapshot);
  void ParticleIndexToId();
  void AverageCoordinates();
  void AssignHosts(const HaloSnapshot_t &halo_snap);
  void PrepareCentrals(HaloSnapshot_t &halo_snap);
  void RefineParticles();
  void UpdateTracks();
  HBTInt size() const
  {
	return Subhalos.size();
  }
  HBTInt GetMemberId(const HBTInt index)
  {
	return Subhalos[index].TrackId;
  }
  const HBTxyz & GetComovingPosition(const HBTInt index) const
  {
	return Subhalos[index].ComovingMostBoundPosition;
  }
  const HBTxyz & GetPhysicalVelocity(const HBTInt index) const
  {
	return Subhalos[index].PhysicalAverageVelocity;
  }
  HBTReal GetMass(const HBTInt index) const
  {
	return Subhalos[index].GetMass();
  }
};
inline HBTInt GetCoreSize(HBTInt nbound)
/* get the size of the core that determines the position of the subhalo.
 * coresize controlled by SubCoreSizeFactor and SubCoreSizeMin.
 * if you do not want a cored center, then
 * set SubCoreSizeFactor=0 and SubCoreSizeMin=1 to use most-bound particle;
 * set SubCoreSizeFactor=1 to use all the particles*/
{
  int coresize=nbound*HBTConfig.SubCoreSizeFactor;
  if(coresize<HBTConfig.SubCoreSizeMin) coresize=HBTConfig.SubCoreSizeMin;
  if(coresize>nbound) coresize=nbound;
  return coresize;
}

//for loading particle properties to calculate corestat
struct ParticleProperty_t
{
// 	  HBTInt ParticleIndex;
  HBTxyz ComovingPosition;
  HBTxyz PhysicalVelocity;
#ifndef DM_ONLY
  HBTReal Mass;
// #ifdef HAS_THERMAL_ENERGY
// 	  HBTReal InternalEnergy;
// #endif
// 	  int Type;
#endif
};

extern hid_t BuildHdfParticlePropertyType();
#define NumPartCore 20
extern void CoreStat(Subhalo_t &Subhalo, const ParticleProperty_t * Particles, HBTInt NumPart);

#endif