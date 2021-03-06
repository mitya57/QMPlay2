#include <BobDeint.hpp>
#include <VideoFilters.hpp>

BobDeint::BobDeint()
{
	addParam("W");
	addParam("H");
}

void BobDeint::filter(QQueue< FrameBuffer > &framesQueue)
{
	int insertAt = addFramesToDeinterlace(framesQueue);
	while (internalQueue.count() >= 2)
	{
		FrameBuffer dequeued = internalQueue.dequeue();
		QByteArray videoFrameData2;

		VideoFrame *videoFrame1 = VideoFrame::fromData(dequeued.data);
		VideoFrame *videoFrame2 = VideoFrame::create(videoFrameData2, w, h);

		for (int p = 0; p < 3; ++p)
		{
			const int linesize = videoFrame1->linesize[p];
			quint8 *src  = videoFrame1->data[p] + linesize;
			quint8 *dst1 = videoFrame1->data[p];
			quint8 *dst2 = videoFrame2->data[p];

			memcpy(dst2, src, linesize); //Copy second line into new frame (duplicate first line in new frame, simple deshake)
			dst2 += linesize;

			const int H = p ? h >> 2 : h >> 1;
			for (int i = 0; i < H; ++i)
			{
				const bool notLast = i != H-1;

				memcpy(dst2, src, linesize);
				dst2 += linesize;
				if (notLast)
				{
					VideoFilters::averageTwoLines(dst2, src, src + (linesize << 1), linesize);
					dst2 += linesize;
				}

				dst1 += linesize;
				if (notLast)
					VideoFilters::averageTwoLines(dst1, src - linesize, src + linesize, linesize);
				else
					memcpy(dst1, src - linesize, linesize);
				dst1 += linesize;

				src  += linesize << 1;
			}

			if (h & 1) //Duplicate last line for odd height
				memcpy(dst2, dst2 - linesize, linesize);
		}

		const bool TFF = isTopFieldFirst(videoFrame1);
		videoFrame1->setNoInterlaced();
		framesQueue.insert(insertAt++, FrameBuffer( TFF ? dequeued.data : videoFrameData2, dequeued.ts));
		framesQueue.insert(insertAt++, FrameBuffer(!TFF ? dequeued.data : videoFrameData2, dequeued.ts + halfDelay(internalQueue.first(), dequeued)));
	}
}

bool BobDeint::processParams(bool *)
{
	w = getParam("W").toInt();
	h = getParam("H").toInt();
	deintFlags = getParam("DeinterlaceFlags").toInt();
	if (w < 2 || h < 4 || !(deintFlags & DoubleFramerate))
		return false;
	return true;
}
