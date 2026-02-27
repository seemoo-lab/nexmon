package com.roger.catloadinglibrary;

import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.Animation;

/**
 * Created by Administrator on 2016/3/30.
 */
public class EyelidView extends View {

    private float progress;
    private boolean isLoading;
    private Paint mPaint;
    private boolean isStop = true;
    private int duration = 1000;
    private ValueAnimator valueAnimator;
    private boolean isFromFull;


    public EyelidView(Context context) {
        super(context);
        init();
    }


    public EyelidView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }


    public EyelidView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }


    public void init() {
        mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mPaint.setColor(Color.BLACK);
        mPaint.setStyle(Paint.Style.FILL);
        setBackground(null);
        setFocusable(false);
        setEnabled(false);
        setFocusableInTouchMode(false);

        valueAnimator = ValueAnimator.ofFloat(0, 1).setDuration(duration);
        valueAnimator.setInterpolator(new AccelerateDecelerateInterpolator());
        valueAnimator.setRepeatCount(Animation.INFINITE);
        valueAnimator.setRepeatMode(ValueAnimator.REVERSE);
        valueAnimator.addUpdateListener(
                new ValueAnimator.AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        progress = (float) animation.getAnimatedValue();
                        invalidate();
                    }
                });
    }


    public void setColor(int color) {
        mPaint.setColor(color);
    }


    public void startLoading() {
        if (!isStop) {
            return;
        }
        isLoading = true;
        isStop = false;
        valueAnimator.start();
    }

    public void resetAnimator() {
        valueAnimator.start();
    }


    public void stopLoading() {
        isLoading = false;
        valueAnimator.end();
        valueAnimator.cancel();
        isStop = true;
    }


    public void setDuration(int duration) {
        this.duration = duration;
    }


    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        if (!isLoading) {
            return;
        }
        if (visibility == View.VISIBLE) {
            valueAnimator.resume();
        } else {
            valueAnimator.pause();
        }
    }

    public void setFromFull(boolean fromFull) {
        isFromFull = fromFull;
    }


    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (!isStop) {
            float bottom = 0.0f;
            if (!isFromFull) {
                bottom = progress * getHeight();
            } else {
                bottom = (1.0f - progress) * getHeight();
            }

            bottom = bottom >= (getHeight() / 2) ? (getHeight() / 2) : bottom;
            canvas.drawRect(0, 0, getWidth(), bottom, mPaint);
        }
    }


    private boolean whenStop() {
        return (isLoading == false && progress <= 0.001f);
    }
}
