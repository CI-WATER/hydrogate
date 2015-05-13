#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/WSCrypt.o \
	${OBJECTDIR}/WSData.o \
	${OBJECTDIR}/WSException.o \
	${OBJECTDIR}/WSHelper.o \
	${OBJECTDIR}/WSLogger.o \
	${OBJECTDIR}/WSPage.o \
	${OBJECTDIR}/WSPageDeleteJob.o \
	${OBJECTDIR}/WSPageDeletePackage.o \
	${OBJECTDIR}/WSPageRequestToken.o \
	${OBJECTDIR}/WSPageRetrieveJobStatus.o \
	${OBJECTDIR}/WSPageRetrievePackageStatus.o \
	${OBJECTDIR}/WSPageRetrieveProgramInfo.o \
	${OBJECTDIR}/WSPageRetrieveTokenExpireTime.o \
	${OBJECTDIR}/WSPageReturnHPCNames.o \
	${OBJECTDIR}/WSPageReturnHPCProgramNames.o \
	${OBJECTDIR}/WSPageSubmitJob.o \
	${OBJECTDIR}/WSPageUploadPackage.o \
	${OBJECTDIR}/WSParameter.o \
	${OBJECTDIR}/WSProxy.o \
	${OBJECTDIR}/WSScheduler.o \
	${OBJECTDIR}/WSSshSession.o \
	${OBJECTDIR}/WSSshSessionPool.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L../cppcms-1.0.4/build -L../cppcms-1.0.4/build/booster -lcppcms -lbooster -lcrypt -lcrypto -lpq -lssh -lssh_threads -luuid -lcurl

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/hydrogate

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/hydrogate: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/hydrogate ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/WSCrypt.o: WSCrypt.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSCrypt.o WSCrypt.cpp

${OBJECTDIR}/WSData.o: WSData.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSData.o WSData.cpp

${OBJECTDIR}/WSException.o: WSException.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSException.o WSException.cpp

${OBJECTDIR}/WSHelper.o: WSHelper.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSHelper.o WSHelper.cpp

${OBJECTDIR}/WSLogger.o: WSLogger.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSLogger.o WSLogger.cpp

${OBJECTDIR}/WSPage.o: WSPage.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPage.o WSPage.cpp

${OBJECTDIR}/WSPageDeleteJob.o: WSPageDeleteJob.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageDeleteJob.o WSPageDeleteJob.cpp

${OBJECTDIR}/WSPageDeletePackage.o: WSPageDeletePackage.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageDeletePackage.o WSPageDeletePackage.cpp

${OBJECTDIR}/WSPageRequestToken.o: WSPageRequestToken.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageRequestToken.o WSPageRequestToken.cpp

${OBJECTDIR}/WSPageRetrieveJobStatus.o: WSPageRetrieveJobStatus.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageRetrieveJobStatus.o WSPageRetrieveJobStatus.cpp

${OBJECTDIR}/WSPageRetrievePackageStatus.o: WSPageRetrievePackageStatus.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageRetrievePackageStatus.o WSPageRetrievePackageStatus.cpp

${OBJECTDIR}/WSPageRetrieveProgramInfo.o: WSPageRetrieveProgramInfo.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageRetrieveProgramInfo.o WSPageRetrieveProgramInfo.cpp

${OBJECTDIR}/WSPageRetrieveTokenExpireTime.o: WSPageRetrieveTokenExpireTime.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageRetrieveTokenExpireTime.o WSPageRetrieveTokenExpireTime.cpp

${OBJECTDIR}/WSPageReturnHPCNames.o: WSPageReturnHPCNames.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageReturnHPCNames.o WSPageReturnHPCNames.cpp

${OBJECTDIR}/WSPageReturnHPCProgramNames.o: WSPageReturnHPCProgramNames.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageReturnHPCProgramNames.o WSPageReturnHPCProgramNames.cpp

${OBJECTDIR}/WSPageSubmitJob.o: WSPageSubmitJob.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageSubmitJob.o WSPageSubmitJob.cpp

${OBJECTDIR}/WSPageUploadPackage.o: WSPageUploadPackage.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSPageUploadPackage.o WSPageUploadPackage.cpp

${OBJECTDIR}/WSParameter.o: WSParameter.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSParameter.o WSParameter.cpp

${OBJECTDIR}/WSProxy.o: WSProxy.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSProxy.o WSProxy.cpp

${OBJECTDIR}/WSScheduler.o: WSScheduler.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSScheduler.o WSScheduler.cpp

${OBJECTDIR}/WSSshSession.o: WSSshSession.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSSshSession.o WSSshSession.cpp

${OBJECTDIR}/WSSshSessionPool.o: WSSshSessionPool.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/WSSshSessionPool.o WSSshSessionPool.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/hydrogate

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
