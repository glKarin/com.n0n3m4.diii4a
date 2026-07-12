#include "idlib/precompiled.h"

#include "OcclusionTest.h"

#include "renderer/tr_local.h"

extern idCVar harm_r_skipOcclusionTesting;

sdOcclusionTestLocal::sdOcclusionTestLocal(void)
	: index(-1),
		world(NULL),
		query(-1)
{
	parms.axis = mat3_identity;
	parms.origin.Zero();
	parms.bb.Zero();
	parms.view = -1;
}

sdOcclusionTestLocal::~sdOcclusionTestLocal(void)
{

}

bool sdOcclusionTestLocal::IsVisible(void) {
	if (harm_r_skipOcclusionTesting.GetBool())
		return true;

	idOcclusionTestJob *test;
	if(query == -1)
	{
		query = occlusionTestManager->Alloc();
		test = occlusionTestManager->Get(query);
		test->UpdateQueryMode(GL_ANY_SAMPLES_PASSED);
		test->Start(idOcclusionTestJob::UT_MANUAL);
	}
	else
	{
		test = occlusionTestManager->Get(query);
		test->Restart();
	}

	return test->lastResult > 0;
}

int sdOcclusionTestLocal::CountVisible(void) {
	if (harm_r_skipOcclusionTesting.GetBool())
		return INT_MAX;

	idOcclusionTestJob *test;
	if(query == -1)
	{
		query = occlusionTestManager->Alloc();
		test = occlusionTestManager->Get(query);
		test->UpdateQueryMode(GL_SAMPLES_PASSED);
		test->Start(idOcclusionTestJob::UT_MANUAL);
	}
	else
	{
		test = occlusionTestManager->Get(query);
		test->Restart();
	}

	return test->lastResult > 0 ? test->lastResult : INT_MAX;
}

void sdOcclusionTestLocal::UpdateOcclusionTest(const occlusionTest_t *testInfo) {
	if(query != -1)
	{
		idOcclusionTestJob *test = occlusionTestManager->Get(query);
		test->UpdateGeometry(testInfo->bb);
		test->UpdatePosition(testInfo->origin, testInfo->axis);
		test->UpdateView(testInfo->view);
	}
}

void sdOcclusionTestLocal::FreeOcclusionTest(void) {
	if(query != -1)
	{
		occlusionTestManager->Free(query);
		query = -1;
	}
}

int sdOcclusionTestLocal::GetResult(void) const {
	if (query == -1)
		return -1;

	return occlusionTestManager->GetResult(query);
}
