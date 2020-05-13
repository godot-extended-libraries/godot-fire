# How to Build a Freaky Good IK System, Easily

This post serves three purposes.

1. To give an overview of Inverse Kinematics methods for the completely uninitiated, or otherwise those just struggling to roll their own.

2. To introduce a novel IK algorithm which is robust, versatile, performant, conceptually simple, and, I believe, superior in most regards to any I've seen outlined elsewhere.

3. To introduce a novel joint constraint mechanism, which is extremely versatile, easy to specify, and quite performant.

Much of this article is informed by my experiences developing [Everything Will Be IK](https://github.com/EGjoni/Everything-WIll-Be-IK), an Open Source Inverse Kinematics Library for Java. If you're looking to implement Inverse Kinematics into a Java Application, please consider extending EWB-IK instead of starting from scratch. Ports to other languages are also always welcome.

This article is divided into four fairly self-contained sections. Feel free to skip around to whichever section interests you.

**Section 0** will give a quick overview of the problem of Inverse Kinematics, and the broad classes of solutions available.

**Section 1** will serve as an in-depth but easy introduction / tutorial on the most popular iterative methods, limitations, and workarounds thereto.

**Section 2** will present a "novel" yet extremely versatile and performant augmentation of a well-worn iterative approach. If you feel you've already paid your dues trying to implement an IK solver, and just want to know how to make something that works, **then you'll want to skip to this section.**

**Section 3** will introduce a new joint constraint mechanism which avoids many of the pitfalls of existing constraint systems for real time applications.

Some parts of **Section 2** will probably be clearer given the perspective offered in **Section 1**, but for the most part this perspective isn't necessary if you care more about results than understanding.

Two last notes before we begin:

- The occipital lobe is a parallel beast, while equations are sequential by nature. So wherever possible, this post will forego equations in favor of the next best thing: animated .gifs. If you prefer equations, please refer to any of the academic articles on the same subject matter.

- Most of the animations throughout this article depict the 2D case for simplicity in conveying the underlying ideas. However, all algorithms depicted generalize readily to 3D unless otherwise stated.

Without further ado, let's begin.

## Section 0: An Overview

Inverse Kinematics is essentially the problem of automatically finding joint orientations to allow a virtual arm or armature to reach one or more targets.

For example, say you have a humanoid armature, and you want to pose it so that its hand reaches for a virtual doorknob. You can either accomplish this via the "Forward Kinematics" route of manually specifying how the spine, shoulder, elbow, and wrist should rotate so that the hand comes into contact with the doorknob, or you can go the _Inverse Kinematics_ route, of just _declaring_ that you want the wrist to be on the doorknob, and letting the computer figure out how it should rotate the spine, shoulder, elbow and wrist to make that happen.

Ideally, the computer would respect some pre-specified constraints on the orientations that the armature's joints are allowed to take on as it attempts whatever task you set before it.We wouldn't want our armature to bend its elbow backwards to reach a doorknob behind it, after all; and would further try to keep the armature's feet firmly planted on the ground the whole time. These additional considerations mean the problem isn't quite as trivial as it might first appear, but we'll explore these in more detail in later sections.

For now, let's get some jargon out of the way so we don't get confused later. There isn't all that much.

- **Chain:** A sequence of bones / joints (henceforth referred to simply as \"bones\") which the IK system will try to determine orientations for.

- **End-Effector**: The outermost bone on a chain --- the one which is trying to meet the target.

- **Target:** A position(/and orientation) we want the end-effector to approach.

We'll learn a couple more terms later, but that's about all we need for now. Reformulating the IK problem in terms of our newly learned jargon, we can say that, in general, the only thing IK solvers do is try to determine the orientations of _bones_ in a **chain** so that the **end-effector(s)** on or descending from the chain meet their corresponding **target(s)**.

Inverse Kinematics is a decades old problem, and by now, approaches to it are numerous enough that it might seem daunting for a newcomer to get a lay-of-the land. No worries, by and large most approaches tend to fall into one of the following four categories:

- **Analytical Methods:** These usually rely on some scheme (either linear programming, or human forethought) to analyze an armature, then devise a closed-form equation that will take a target as input, and return as output the optimal pose a chain must adopt so that its end-effector meets that target. These methods are blazing fast at runtime, but are by no means general, and often require very careful reasoning in advance about the specifics of the armature they will be applied to. They tend to be most popular in robotics applications due to their provability.

- **Optimization Methods:** These usually attempt to formulate the problem as one of finding some minima in a hyperdimensional space of rotations and translations. These often need to rely on complicated (and brittle) mathematical tricks for inverting uninvertible matrices (Jacobian Pseudo-inverses, Hessians, Dampened Least Squares filtering, etc). They're most popular in professional graphics applications, where quality and user guided predictability are of greater concern than speed or versatility.

- **Physics Based Methods:** These often rely on a physics solver to treat the armature as a "ragdoll." Physics based methods tend to be fairly fast, and fairly versatile, but come at a considerable cost to quality and control (if you treat your armature as a ragdoll, then you're liable to get ragdoll-like results) and are very prone to getting stuck in local minima. They are most popular in gaming applications where speed and compatibility with existing systems is of greater concern than quality. In recent years, these have been losing some ground to iterative methods as audiences have grown to expect greater levels of realism.

- **Iterative Methods:** These are among the most popular for real time applications. They, and variants of them, tend to offer relatively natural results fairly quickly, but they often lack versatility, and are prone to instabilities. They are most popular in real time motion-tracking and gaming applications.

The rest of this article will focus primarily on iterative methods. If you're reading this with an eye towards robotics applications, this _may_ not be the article for you (or, at the very least, you'll want to put due effort to make whatever you learn here safe for physical use). However, for (realtime or non-realtime) graphics and virtual reality applications, I hope Section 2 in particular will prove extremely useful to anyone looking for alternatives to the weaknesses of existing optimization or iterative methods. I have little experience integrating IK with existing physics systems, so unfortunately I can't make any qualified guarantees as to the suitability of the algorithms presented here to systems which must be subject to virtual physical forces.

## Section 1: Iterative Methods

One of the most common iterative approaches to the Inverse Kinematics problem is the rather elegant _Cyclic Coordinate Descent_ (CCD), depicted in the animation below.

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_66cf58f5bc31cfe4.gif){width="572" height="346"}

In this animation, each dark blue "pick" represents a bone in the chain. The orange blob represents our end effector, and the red X represents our target.

All CCD does is (starting from the outermost bone on the chain):

1. Run a line (depicted in cyan) through the base of the bone to the end-effector

2. Run another line (depicted in magenta) from the base of the bone to the target

3. Rotate the bone + cyan line so that the cyan line aligns with the magenta line.

This process is repeated for as many iterations as it takes to converge (or, at least for whatever number of iterations you have the patience for, though convergence is usually reasonably quick).

The very minimalist implementation of CCD depicted above is all well and good if you're trying to solve for something like a tail or a snake, where bones can be allowed to rotate approximately freely in approximately any direction, but for most use cases, we don't really want our bones to have _that_ much freedom. If we want to solve for an arm, for example, we wouldn't want any solutions that require the elbow to bend backwards. To avoid grotesque solutions, we need **Constraints.**

- **Constraint:** A limit imposed on the orientations of a bone relative to its parent bone.

In 3D, constraints aren\'t as straightforward as one might naively assume (see section 3 for an overview and suggestions), but in the 2D case can be represented simply, and suffice to depict many (but not all) of the problems we can expect to run into when generalizing to 3D. So we\'ll just limit ourselves to 2D constraints for now.

In the picture below, the constraints are represented as green triangular blobs. Each bone can be oriented in any direction that remains within the confines of its green blob.

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_fbda1eb1ba331172.png){width="572" height="286"}

As it turns out, with minor modification, CCD handles most constraints without much problem. We simply run CCD on each bone as usual, but before moving on to the next bone, we "snap" the bone we just rotated so that it's back within the confines of its constraint. In the animation below, the constraint turns a panicked red when a bone goes beyond its confines, immediately after which, the bone is snapped back within its confines, and the algorithm continues on to the next bone.

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_b5715fb6a6b71d50.gif){width="572" height="346"}

As we can see, the reason everything still works out is that constraints on bones at the end of the chain are compensated for (where possible) by the freedom still available to bones closer to the root of the chain.

You might note that the above solution has a distinctly "unnatural" look. In practice, this sort of unnaturalness is trivially avoided by the introduction of a simple **dampening** parameter.

- **Dampening:** Limits how many degrees a bone can rotate in a single iteration of CCD.

In effect, dampening prevents any child bone from bearing too much of the brunt of a rotation at any given step in the algorithm, thereby resulting in a more evenly distributed curve along the chain. The lower you set the dampening parameter, the more "natural" CCD solutions tend to look; however, this also means that CCD will take more iterations to converge on a solution. How much speed you're willing to sacrifice for how natural a look will obviously depend on your use-case, but as a point of reference, a four bone chain of CCD will easily complete 10 iterations in under 0.1 milliseconds on modern (as of 2019) hardware.

If your use-case is such that you only need to worry about a **single** end-effector on a single constrained or unconstrained chain with a **position** target, there isn't really much reason to look any further than CCD.

But what if you want to solve for multiple end-effectors? For example, consider the armature below:

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_cc4ef32d15fd4490.png){width="572" height="286"}

We want to get end-effector 1 to reach for x~1~, and end-effector 2 to reach for x~2~.

CCD could easily solve for _either_ one of them, but in naively solving for _both,_ CCD would need to say that bone B would need to point in two different directions at once. One direction to allow for C~1~ and D~1~ to reach for x~1~, and another direction to allow for C~2~ and D~2~ to reach for x~2~.

So what can we do here? Well, one thing we could do is abandon CCD and opt instead for _Forward and Backward Reaching Inverse Kinematics_ (FABRIK) --- so named in part due to how it works, and presumably in part as a nod to the algorithm's origins in rope simulation. It goes as follows:

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_f94c2440e49ae453.gif)

FABRIK can handle pretty much any number of end-effectors with ease, so it can be a good fit if you need to apply quick IK to jellyfish or octopusses or vines. But it struggles horribly if you try to use it in conjunction with constraints. As a result, it's a really poor fit for most humanoid applications.

The original paper introducing FABRIK goes through some length explaining its constraint scheme, so if you've tried implementing it before you might be wondering if you're dumb for not being able to get it to work right. The answer to that is "no." At worst, you're dumb for thinking it could be done in the first place (I speak from personal experience). Constraining FABRIK leads to deadlocks.

So as things stand, it looks like we have to pick and choose. We can either have a system that behaves well under constraints, but only supports one end-effector per chain; or we can have a system that supports multiple end-effectors, but doesn't play well with constraints.

If we want the best of both worlds, we'll have to go slightly off-script. Can we maybe try some sort of hybrid approach? Take the best of FABRIK and combines it with the best of CCD?

Indeed, we can actually come up with at least two such approaches.

Let's think for a moment about what exactly CCD is doing every time it rotates a bone. If we consider _all_ of the orientations available for a bone to rotate to, we notice that, in effect, CCD always picks the orientation which minimizes the distance between the chain\'s end-effector and its target.

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_fda38426175f5a2b.png)

Part of the reason that CCD still works well when we constrain it is that (presuming we've designed our constraints well), this holds true even after snapping a bone back within the confines of its constraint. That is to say, the chain\'s end-effector will still be as close as it can to the target after accounting for the orientations our constraints disallow. So whatever hybrid approach we adopt will want to retain something similar to this property.

Next, let's consider what FABRIK is doing to handle multiple end-effectors so well. The critical steps are the ones depicted below:

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_f147e5881c69c301.gif){width="572" height="286"}

Essentially, what FABRIK does to resolve two conflicting options is average them, and then simply approach the average.

Modifying CCD to work similarly should then be trivial. We could just take the average position of our first and second end-effectors, and rotate our bone so as to bring it toward the average position of our first and second targets. ![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_63b2f3f38adfae1.gif)

Alternatively, we could take the _rotation_ which minimizes the distance between our first end-effector and our first target, and average it with the rotation that minimizes the distance between our second end-effector and our second target.

But we have to be very careful with that second option. More specifically, we need to make sure we dampen our rotations _before_ averaging them. This is because "averaging" rotations isn't a valid procedure in 3D. It works well enough if rotations are "close" to one another, but the more disparate the rotations we're trying to average, the wackier things get. If we dampen our rotations first, we ensure that they remain sufficiently "close" to one another to mitigate most problems we might expect from our ad-hoc "averaging" of 3D rotations.

If you settle on this scheme, you might also find it helpful to apply a normalization procedure after dampening, so that your largest rotation ends up at the dampening limit, and your smaller rotations are scaled down in proportion to the largest one. This way, you can retain information about how _much_ any target is "pulling" in a particular direction.

For the most part though, I suggest not adopting either of these solutions. I've presented these options primarily for posterity, and to lay the groundwork for a more reasonable and general approach. In practice, they have serious shortcomings.

For one thing, you're likely to encounter stability issues. And for another, FaBRIK, CCD, and even our hybrid approaches, often prove insufficient for many real world use-cases due to a lack of any sensible way to handle **orientation** targets. If you\'re sure your use case shouldn\'t present much of a problem, then by all means, go implement any of what\'s been presented above.

If you\'re not sure, then try the following experiment, and then take a moment to consider if any part of it maps onto the problem you\'re trying to solve.

Hold your hand in front of your face so that your palm is facing you, and your fingers are pointing upwards toward the ceiling (or sky, or awning or whatever).

Make a mental note of where your elbow is.

Next, keep your palm in the same position relative to your face, but rotate it 180 degrees so that your palm is still facing you, but your fingers are now pointing downward. If you can't do it, simply stop worrying about how silly you look.

Notice how radically different a position your elbow takes on? Likely, even your shoulder, collarbone, and possibly your spine had to adjust to compensate for the various constraints on your wrist, elbow and shoulder joints.

Unfortunately, we can't handle this very well with either CCD or FaBRIK. We can throw in a few tricks, but most of these don't work too well as a general solution, which can lead to some [[pretty horrifying results]{.underline}](https://youtu.be/D990PJL_CqQ?t=679) if we apply them to situations where an armature is free to do basically whatever a user specifies.

The solution to this, turns out to be surprisingly simple.

## Section 2:

Consider the following chain. We want the y and z axes of the end-effector (highlighted in orange) to match both the position _and_ orientation of the target axes (\"circled\" in red).

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_10e6c5339cfbe2c7.png)

The position of the end-effector's origin and the target's origin already match up, so there's nothing more to do there. But how do we get the orientations to match? One thing we could do is naively rotate the end effector to match the target's orientation, but our constraints on the end-effector don't actually allow for this. Once we snap the bone back within the confines of its constraints, we end up right back where we started.

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_a3529d17dd261991.gif)

What we'd really like is (similarly to the elbow experiment in section 1) for the bones further down the chain to compensate for the limits imposed by on the end-effector\'s by its constraint so that the end effector can maintain a valid orientation.

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_2d695078f6364dd8.gif)

We could try to fix this by next solving for the y-axis of the end-effector and the y-axis of the target, and then the z-axis of the end-effector and the z-axis of the target, and repeating over and over again in the hopes that the chain converges. But convergence isn't guaranteed, nor is this approach especially performant.

In Section 1 (which you don\'t have to go back and read if you skipped it), we saw that CCD works by always minimizing the distance between the end-effector and the target.

![](How%20to%20Build%20a%20Freaky%20Good%20IK%20System,%20Easily%20(wip)_html_fda38426175f5a2b.png)

We also saw that if we want to adapt CCD to multiple end-effectors, we can adopt some sort of averaging scheme. That is, we could either rotate the bone under consideration to bring the average of all end effectors toward the average of all targets, or we can store each rotation bringing each end-effector to its corresponding target, and then find the average of those (dampened and preferably normalized) rotations.

Unfortunately, trying to adapt either approach to the case of orientation targets is likely to lead to stability issues at best, and singularity issues at worst. It would probably take you less effort to imagine reasons why this is true than it would take me to illustrate them, so I won't bother, but the takeaway is this:

1. Finding the average of the rotations that minimizes the distance between the end-effectors and their corresponding targets = BAD :(

2. Finding the rotation that minimizes the distance between the average end-effector and the average target = ALSO BAD :(

3. Finding the rotation that minimizes the average distance between all end-effectors and their corresponding targets = GOOD :)

(If you\'re thinking in 2D, it might not be immediately obvious that the 3 options above are distinct, so you may wish to take a moment to consider the differences in 3D)

Ultimately, adopting option 3 means we're still just trying to minimize a single value --- specifically, the Root Mean Square Error --- but it's a value which has encoded within it the requirements of every target we're trying to solve for.

So how do we find the Root Mean Squared Error?

As it turns out, there are a few ways. They've been widely known to the Bioinformatics and Crystallography communities since the late 70's as algorithms for minimizing the "Root Mean Squared Deviation of Atomic Positions," though by some implausible lack of cross-pollination they've been weirdly neglected as potential solutions to the Inverse Kinematics problem.

The implementation of the earliest of thesw algorithms (Kabsch alignment) is a bit involved, and somewhat costly to compute; often requiring the eigendecomposition of a matrix. However, more recent incarnations (particularly, the Quaternion Characteristic Polynomial method) are quite performant, and easy to implement without reliance on external libraries.

Armed with QCP (or any other algorithm for minimizing the RMSD between effector-target pairs), we can get the best of all worlds. Multiple end-effectors, with position _and_ based targets, _and_ stability under constraints.

To solve for the orientation and position of a single end-effector, we treat the effector as being made of four points: one representing the effector's position, and three representing its x, y, and z directions. Then, if we represent its target in the same way, we can run QCP on the effector and target point-pairs to find the rotation that minimizes the average distance between all four point-pairs. To solve for _multiple_ end effectors, we just toss them and their corresponding targets into the mix. Don\'t worry too much about scaling it up, these algorithms are designed for dealing with hundreds of atoms, so you have plenty of room to do IK on like, a weird hyper-squid, or whatever.

Interestingly, this approach constitutes a generalization of CCD. To see why, consider what happens if we run QCP on a single chain with a single position-based end-effector.

Because both CCD and QCP will, for each bone, just find the rotation that minimizes the average distance between the end-effector and its target, we see that QCP will necessarily give the same exact result as CCD.

QCP and similar alignment algorithms also come with a number of other convenient advantages.

First, they allow us to incorporate "weights" on our effector-target pairs, which means we can easily tell our solver that one target is more important than another.

Second, by their very nature, these algorithms calculate the RMSD for their solution, which allows us to easily do fancy things like early termination or attempting to solve from a different starting pose if we suspect we're stuck in a local minima. QCP in particular grants us a shortcut, where we can check the RMSD in something like half the time it would take to also compute the rotation which gives us that RMSD.

And finally, alignment algorithms compute not only the rotation required to minimize the RMSD, but also the translation. While we usually, want to ignore the translational component so that ligaments in our armatures don't detach, it's often useful to consider it with regard to the special case of an armature's root bone. Particularly, when we don't want to anchor the root to a particular position in space. For example, we could easily make something like a humanoid armature with end effectors on _just_ the hands, head, and feet; leaving the armature's hips free to adopt whatever position and orientation would most readily put those points in reach.

This RMSD approach to Inverse Kinematics doesn't have as long or established a history as CCD (largely because, as far as I can tell, it didn't exist until last Thursday), but alignment algorithm implementations are widely available in a number of languages for a number of Molecular Dynamics libraries, and QCP in particular is self-contained enough that it can easily be shamelessly stolen --- so it shouldn't take more than a couple of hours at most to roll your own IK solution using what you've learned here.

In my experiments thus far, the alignment algorithm approach has managed to handle basically everything I've thrown at it with ease, speed, and grace. So I strongly suggest trying it first, and only worrying about the other approaches mentioned above if for some reason it doesn't suit your purposes. (And if you do find some usecase where it fails but other iterative approaches work, please comment here for the benefit of others).

## Section 3:
