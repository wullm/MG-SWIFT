#Set up monofonIC
git clone -b MG https://github.com/lancelotlibrary/MG-monofonic
cd monofonic
mkdir build; cd build
ccmake ..
make
mpirun -np 16 ./monofonIC <path to config file> #Obtain initial conditions with this command
  
#Set up VELOCIraptor libraries
git clone https://github.com/pelahi/VELOCIraptor-STF
cd VELOCIraptor-STF
mkdir build-sp
mkdir build-mp
cd build-sp
cmake ../ -DVR_USE_HYDRO=OFF -DVR_MPI=OFF -DVR_USE_SWIFT_INTERFACE=ON -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_BUILD_TYPE=Release -DVR_HDF5=ON
make -j
cd ..
cd build-mp
cmake ../ -DVR_USE_HYDRO=OFF -DVR_MPI=ON -DVR_USE_SWIFT_INTERFACE=ON -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_BUILD_TYPE=Release -DVR_HDF5=ON

#Configure swift (change path to VELOCIraptor libraries
cd swiftsim
./autogen.sh
./configure --disable-compiler-warnings --disable-doxygen-doc --with-tbbmalloc --with-parmetis --enable-lightcone --with-velociraptor=/home/oleksii/VELOCIraptor-STF/build-sp/src/ --with-velociraptor-mpi=/home/oleksii/VELOCIraptor-STF/build-mp/src/
make


#SLURM script to run on a large HPC systems (This script will run on 8 nodes each with 2x18 core processors for a total of 288 cores. 
#Change number of nodes, processor cores and tasks per nodes accordingly)

#SBATCH -N 8  # Number of nodes to run on
#SBATCH --tasks-per-node=2  # This system has 2 chips per node

mpirun -n 16 ../../swift --cosmology --self-gravity --power --velociraptor --threads=18 --pin parameter.yml

