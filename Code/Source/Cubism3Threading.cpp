#include "StdAfx.h"

#include "Cubism3UIComponent.h"

#include <AzCore/std/sort.h>

namespace Cubism3 {
	#ifdef CUBISM3_THREADING
	Cubism3UIInterface::Threading Cubism3UIComponent::m_threadingOverride = CUBISM3_THREADING;
	#else
	Cubism3UIInterface::Threading Cubism3UIComponent::m_threadingOverride = Cubism3UIInterface::Threading::DISABLED;
	#endif

	//Threading stuff
	///base thread
	Cubism3UIComponent::DrawableThreadBase::DrawableThreadBase(AZStd::vector<Drawable*> *drawables) {
		TLOG("[Cubism3] DTB - Base Init Start.");
		this->m_drawables = drawables;
		this->m_renderOrderChanged = false;
		this->m_canceled = false;
		TLOG("[Cubism3] DTB - Base Init End.");
	}

	void Cubism3UIComponent::DrawableThreadBase::Cancel() {
		TLOG("[Cubism3] DTB - Base Cancel Start.");
		this->WaitTillReady();
		this->m_canceled = true;
		this->Notify();
		TLOG("[Cubism3] DTB - Base Cancel End.");
	}

	void Cubism3UIComponent::DrawableThreadBase::WaitTillReady() {
		TLOG("[Cubism3] DTB - Base Wait Start.");
		this->mutex.Lock();
		this->mutex.Unlock();
		TLOG("[Cubism3] DTB - Base Wait End.");
	}

	///single thread
	void Cubism3UIComponent::DrawableSingleThread::Run() {
		TLOG("[Cubism3] DST - Thread Run Start.");
		while (!this->m_canceled) {
			TLOG("[Cubism3] DST - Waiting...");
			this->Wait();
			TLOG("[Cubism3] DST - Running.");
			this->mutex.Lock();
			if (this->m_canceled) break;
			this->m_renderOrderChanged = false;
			csmUpdateModel(this->m_model);

			for (Drawable* d : *m_drawables) d->update(this->m_model, this->m_transformUpdate, this->m_renderOrderChanged);

			if (this->m_renderOrderChanged)
				AZStd::sort(
					this->m_drawables->begin(),
					this->m_drawables->end(),
					[](Drawable * a, Drawable * b) -> bool {
						return a->renderOrder < b->renderOrder;
					}
				);

			csmResetDrawableDynamicFlags(this->m_model);
			this->mutex.Unlock();
		}
		this->mutex.Unlock();
		TLOG("[Cubism3] DST - Thread Run End.");
	}

	///multithread
	//each drawable gets a thread.
	/*
	Cubism3UIComponent::DrawableMultiThread::DrawableMultiThread(AZStd::vector<Drawable*> &drawables, AZ::Matrix4x4 &transform, unsigned int limiter = CUBISM3_MULTITHREAD_LIMITER) : DrawableThreadBase(drawables, transform) {
	for (Drawable * d : drawables) {
	SubThread * t = new SubThread(d, this);
	t->Start();
	this->m_threads.push_back(t);
	}
	this->m_threads.shrink_to_fit();

	this->semaphore = new CrySemaphore(limiter); //limiter for subthread execution
	}
	Cubism3UIComponent::DrawableMultiThread::~DrawableMultiThread() {
	for (SubThread* t : this->m_threads) {
	t->Cancel();
	t->WaitTillDone();
	t->WaitForThread();
	}
	this->m_threads.clear();

	this->WaitTillReady();
	this->Cancel();
	}

	void Cubism3UIComponent::DrawableMultiThread::Run() {
	while (!this->m_canceled) {
	this->Wait();
	this->mutex.Lock();
	if (this->m_canceled) break;

	this->rwmutex.WLock();
	this->m_drawOrderChanged = this->m_renderOrderChanged = false;
	this->rwmutex.WUnlock();

	csmUpdateModel(this->m_model);

	for (SubThread * t : this->m_threads) t->Notify();
	for (SubThread * t : this->m_threads) t->WaitTillReady(); //make sure every thread is done before unblocking.
	this->mutex.Unlock();
	}
	this->mutex.Unlock();
	}

	bool Cubism3UIComponent::DrawableMultiThread::DrawOrderChanged() {
	bool ret = false;
	this->rwmutex.RLock();
	ret = this->m_drawOrderChanged;
	this->rwmutex.RUnlock();
	return ret;
	}
	bool Cubism3UIComponent::DrawableMultiThread::RenderOrderChanged() {
	bool ret = false;
	this->rwmutex.RLock();
	ret = this->m_renderOrderChanged;
	this->rwmutex.RUnlock();
	return ret;
	}

	///multithread - sub thread
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Run() {
	while (!this->m_canceled) {
	this->Wait();
	this->mutex.Lock();
	this->m_dmt->semaphore->Acquire();

	if (this->m_canceled) break;
	bool drawOrderChanged = false, renderOrderChanged = false;
	this->m_d->update(m_dmt->GetModel(),*m_dmt->GetTransform(), m_dmt->GetTransformUpdate(), drawOrderChanged, renderOrderChanged);

	m_dmt->rwmutex.WLock();
	if (drawOrderChanged) this->m_dmt->SetDrawOrderChanged(true);
	if (renderOrderChanged) this->m_dmt->SetRenderOrderChanged(true);
	m_dmt->rwmutex.WUnlock();
	this->m_dmt->semaphore->Release();
	this->mutex.Unlock();
	}
	this->mutex.Unlock();
	this->m_dmt->semaphore->Release();
	}
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Cancel() {
	this->WaitTillDone();
	this->m_canceled = true;
	this->Notify();
	}

	void Cubism3UIComponent::DrawableMultiThread::SubThread::WaitTillDone() {
	this->mutex.Lock();
	this->mutex.Unlock();
	}
	*/

	///multithread alt
	//limited number of threads, each thread updates a drawable.
	Cubism3UIComponent::DrawableMultiThread::DrawableMultiThread(AZStd::vector<Drawable*> *drawables, unsigned int limiter) : DrawableThreadBase(drawables) {
		TLOG("[Cubism3] DMT - Multi Init Start.");
		m_threads = new SubThread*[limiter];
		this->numThreads = limiter;

		for (int i = 0; i < limiter; i++) {
			m_threads[i] = new SubThread(this);
			m_threads[i]->Start();
		}
		TLOG("[Cubism3] DMT - Multi Init End.");
	}
	Cubism3UIComponent::DrawableMultiThread::~DrawableMultiThread() {
		TLOG("[Cubism3] DMT - Multi Destroy Start.");
		for (int i = 0; i < numThreads; i++) {
			this->m_threads[i]->Cancel();
			this->m_threads[i]->WaitTillReady();
			this->m_threads[i]->WaitForThread();
			delete this->m_threads[i];
		}
		delete this->m_threads;

		this->Cancel();
		this->WaitTillReady();
		TLOG("[Cubism3] DMT - Multi Destroy End.");
	}

	void Cubism3UIComponent::DrawableMultiThread::Run() {
		TLOG("[Cubism3] DMT - Run Start.");
		while (!this->m_canceled) {
			TLOG("[Cubism3] DMT - Waiting...");
			this->Wait();
			TLOG("[Cubism3] DMT - Running.");
			this->mutex.Lock();
			if (this->m_canceled) break;

			this->rwmutex.WLock();
			this->m_renderOrderChanged = false;
			this->rwmutex.WUnlock();

			this->dMutex.Lock();
			this->nextDrawable = 0;
			this->dMutex.Unlock();

			csmUpdateModel(this->m_model);

			for (int i = 0; i < numThreads; i++) this->m_threads[i]->Notify(); //wake up worker threads
			for (int i = 0; i < numThreads; i++) this->m_threads[i]->WaitTillReady(); //wait until the worker threads are done

			if (this->m_renderOrderChanged)
				AZStd::sort(
					this->m_drawables->begin(),
					this->m_drawables->end(),
					[](Drawable * a, Drawable * b) -> bool {
						return a->renderOrder < b->renderOrder;
					}
				);

			csmResetDrawableDynamicFlags(this->m_model);

			this->mutex.Unlock();
		}
		this->mutex.Unlock();
		TLOG("[Cubism3] DMT - Run End.");
	}

	bool Cubism3UIComponent::DrawableMultiThread::RenderOrderChanged() {
		TLOG("[Cubism3] DMT - Render Order Changed Start.");
		bool ret = false;
		this->rwmutex.RLock();
		ret = this->m_renderOrderChanged;
		this->rwmutex.RUnlock();
		return ret;
		TLOG("[Cubism3] DMT - Render Order Changed End.");
	}

	Cubism3UIComponent::Drawable * Cubism3UIComponent::DrawableMultiThread::GetNextDrawable() {
		TLOG("[Cubism3] DMT - GetNextDrawable Start.");
		if (!this->m_canceled) {
			if (this->nextDrawable >= this->m_drawables->size()) {
				TLOG("[Cubism3] DMT - GetNextDrawable End - No Drawable.");
				return nullptr;
			}
			Drawable * ret = this->m_drawables->at(this->nextDrawable);
			this->nextDrawable++;
			TLOG("[Cubism3] DMT - GetNextDrawable End - Has Drawable.");
			return ret;
		}
		TLOG("[Cubism3] DMT - GetNextDrawable End - Canceled.");
		return nullptr;
	}

	///multithread alt - sub thread
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Cancel() {
		TLOG("[Cubism3] DMT - SubThread[%i] - Cancel Start.", this->m_threadId);
		this->WaitTillReady();
		this->m_canceled = true;
		this->Notify();
		TLOG("[Cubism3] DMT - SubThread[%i] - Cancel End.", this->m_threadId);
	}
	void Cubism3UIComponent::DrawableMultiThread::SubThread::Run() {
		TLOG("[Cubism3] DMT - SubThread[%i] - Run Start.", this->m_threadId);
		Drawable * d = nullptr;
		while (!this->m_canceled) {
			TLOG("[Cubism3] DMT - SubThread[%i] - Waiting...", this->m_threadId);
			this->Wait();
			TLOG("[Cubism3] DMT - SubThread[%i] - Running.", this->m_threadId);
			this->mutex.Lock();
			if (this->m_canceled) break;

			d = nullptr;

			//get first drawable to update
			this->m_dmt->dMutex.Lock();
			d = this->m_dmt->GetNextDrawable();
			this->m_dmt->dMutex.Unlock();
			while (d) {
				if (this->m_canceled) break;
				bool renderOrderChanged = false;

				//update the drawable
				d->update(m_dmt->GetModel(), m_dmt->GetTransformUpdate(), renderOrderChanged);

				m_dmt->rwmutex.WLock();
				if (renderOrderChanged) this->m_dmt->SetRenderOrderChanged(true);
				m_dmt->rwmutex.WUnlock();

				//get next drawable to update
				this->m_dmt->dMutex.Lock();
				d = this->m_dmt->GetNextDrawable();
				this->m_dmt->dMutex.Unlock();
			}

			if (this->m_canceled) break;
			this->mutex.Unlock();
		}
		this->mutex.Unlock();
		TLOG("[Cubism3] DMT - SubThread[%i] - Run End.", this->m_threadId);
	}

	void Cubism3UIComponent::DrawableMultiThread::SubThread::WaitTillReady() {
		TLOG("[Cubism3] DMT - SubThread[%i] - Wait Start.", this->m_threadId);
		this->mutex.Lock();
		this->mutex.Unlock();
		TLOG("[Cubism3] DMT - SubThread[%i] - Wait End.", this->m_threadId);
	}
}