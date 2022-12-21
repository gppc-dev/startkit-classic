# Submission Instruction

## Login with your github account

Login the [competition website](https://gppc.search-conference.org/grid) with a github account, and we will automatically create an private github repo for you. 
The repo will be the place that you submit codes to. You can click "My Repo" to open your github repo page.

## Make your first submission

Clone the starter kit, which includes a basic Theta-star implementation in **anyangle branch**. 

```
$ git clone https://github.com/gppc-dev/startkit.git
$ cd startkit
$ git checkout anyangle
```
Add your remote competition private repo to the startkit local repo, and push the basic a-star to the master of your remote private repo.
```
$ git remote add contest_server git@github.com:your_repo_address
$ git push contest_server anyangle:master
```
Finally click "Evaluate my codes" button on the competition website! The server then evaluates the implementation in the repo.

## Implement your algorithm

Read the Problem_Definition.md to check the problem definitions.

Read the start kit README.md to know what files you should (not) modify, and where you should implement your algorithm.

When your implementation is ready to evaluate, add, commit, and push all your changes by running following commands in your local repo:
```
$ git add *
$ git commit -m "Some message to describe this commit."
$ git push contest_server anyangle:master
```

If your implementation support multi-thread preprocessing, tick the "My Entry Support Multi-thread Preprocessing" option. 
If ticked, the preprocessing server will assign 4 cpus for preprocessing (otherwise only 1 cpu is assigned).

Then click the "Evaluate my codes" button on the competition website to evaluate your new submission.

## Track evaluation progress and history submission

Click the "My Submissions" button to see your submission history. 
Click an entry in the history to see details and track the progress of a running evaluation. 


